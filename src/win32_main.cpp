#include "render.h"

#include "utils/stub.hpp"

// TODO(apirogov): slim windows.h a little. Include only things that we need 
#include <windows.h>
#include <dsound.h>
#include <math.h>
#include <Xinput.h>
#include <xaudio2.h>

#include <cstdint>
#include <filesystem>
#include <type_traits>
#include <utility>

namespace fs = std::experimental::filesystem;

constexpr float Pi = 3.14159265358979f;

DWORD WINAPI XInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState);
using x_input_get_state = decltype(XInputGetState);
static x_input_get_state * DynamicXInputGetState = [](DWORD dwUserIndex, XINPUT_STATE* pState)->DWORD{return ERROR_DEVICE_NOT_CONNECTED;};

DWORD WINAPI XInputSetState(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);
using x_input_set_state = decltype(XInputSetState);
static x_input_set_state * DynamicXInputSetState = [](DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)->DWORD{return ERROR_DEVICE_NOT_CONNECTED;};

std::pair<int, int> win32GetWindowDimension(HWND window) {
    RECT clientRect;
    GetClientRect(window, &clientRect);
    const int width = clientRect.right - clientRect.left;
    const int height = clientRect.bottom - clientRect.top;
    return {width, height};
}

struct Win32OffscreenBuffer {
    bool running;
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int bytes_per_pixel;
    int pitch;
};

static bool running;
static Win32OffscreenBuffer global_backbuffer;
static LPDIRECTSOUNDBUFFER SecondaryBuffer;

#define NAME(func) #func

static void Win32ResizeDeviceIndependentBitmap(Win32OffscreenBuffer * buffer, const LONG width, const LONG height) {

    if(buffer->memory)
        VirtualFree(buffer->memory, 0, MEM_RELEASE);

    buffer->width = width;
    buffer->height = height;
        
    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;
    buffer->bytes_per_pixel = 4;

    const int bitmap_memory_size = buffer->bytes_per_pixel * buffer->width * buffer->height;
    
    buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);

    buffer->pitch = buffer->width * buffer->bytes_per_pixel;
}

static void Win32UpdateWindow(HDC device_context,
                        int window_width, int window_height,
                              const Win32OffscreenBuffer& buffer,
                              const LONG x,
                              const LONG y)
{
    int result = StretchDIBits(device_context,
                               0, 0, window_width, window_height,
                               0, 0, buffer.width, buffer.height,
                               buffer.memory,
                               &buffer.info,
                               DIB_RGB_COLORS,
                               SRCCOPY);
}

static void Win32InitXInput() {
    HMODULE x_input_library = ::LoadLibrary("xinput1_3.dll");
    if(x_input_library) {
        DynamicXInputGetState = (x_input_get_state *)::GetProcAddress(x_input_library, NAME(XInputGetState));
        DynamicXInputSetState = (x_input_set_state *)::GetProcAddress(x_input_library, NAME(XInputSetState));
    }
}

static void Win32InitXAudio2() {
    IXAudio2 * x_audio2 = nullptr;
    HRESULT error;
    if(FAILED(error = XAudio2Create(&x_audio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
        return;
    IXAudio2MasteringVoice* master_voice = nullptr;
    if ( FAILED(error = x_audio2->CreateMasteringVoice( &master_voice ) ) )
        return;
}


struct SoundData{
    int32_t SamplesPerSecond = 48000;
    int32_t ToneHz = 256;
    int16_t ToneVolume = 3000;
    uint32_t RunningSampleIndex = 0;
    int32_t WavePeriod = SamplesPerSecond/ToneHz;
    int32_t HalfWavePeriod = WavePeriod/2;
    int32_t BytesPerSample = sizeof(int16_t)*2;
    int32_t SecondaryBufferSize = SamplesPerSecond*BytesPerSample;
} global_sound_data;

static void  Win32InitDirectSound(HWND Window, int32_t SamplesPerSecond, int32_t BufferSize) {
    HMODULE DSoundLibrary = ::LoadLibrary("dsound.dll");
    using direct_sound_create = HRESULT WINAPI (LPGUID lpGuid, LPDIRECTSOUND * ppDS, LPUNKNOWN  pUnkOuter );
    if(DSoundLibrary) {
        direct_sound_create * DirectSoundCreate = (direct_sound_create *)::GetProcAddress(DSoundLibrary, NAME(DirectSoundCreate));
        LPDIRECTSOUND DirectSound;
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.cbSize = 0;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample ) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;

            if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0))) {
                    HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
                    if(SUCCEEDED(Error)) {
                        OutputDebugString("Primary buffer was set.\n");
                    } else {
                        // TODO: Handle error
                    }
                } else {
                    // TODO: Handle error
                }
            } else {
                // TODO: Handle error
            }

            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;

            HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &SecondaryBuffer, 0);
            if(SUCCEEDED(Error)) {
                OutputDebugString("Secondary buffer created successfully.\n");
                SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            } else {
                // TODO: Handle error
            }


        } else {
            // TODO: Handle error
        }
    } else {
        // TODO: Handle error
    }
}

static void FillSoundRegion(VOID * region, DWORD region_size, SoundData * sound_data) {
    DWORD region_sample_count = region_size/sound_data->BytesPerSample;
    int16_t * sample_out = (int16_t*) region;
    for(DWORD sample_index = 0; sample_index < region_sample_count; ++sample_index) {
        float t = 2.0f * Pi * (float)sound_data->RunningSampleIndex / (float)sound_data->WavePeriod;
        float sine_value = sinf(t);
        int16_t sample_value = (int16_t)(sine_value * sound_data->ToneVolume);
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;

        ++sound_data->RunningSampleIndex;
    }
}

static void PlaySineWave(SoundData * sound_data) {
    // Test DirectSound
    DWORD PlayCursor;
    DWORD WriteCursor;
    if(SUCCEEDED(SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))) {
        DWORD BytesToLock = sound_data->RunningSampleIndex * sound_data->BytesPerSample % sound_data->SecondaryBufferSize;
        DWORD BytesToWrite = {};
        if(BytesToLock == PlayCursor) {
            BytesToWrite = 0;
        } else if (BytesToLock > PlayCursor) {
            BytesToWrite = (sound_data->SecondaryBufferSize - BytesToLock);
            BytesToWrite += PlayCursor;
        } else {
            BytesToWrite = PlayCursor - BytesToLock;
        }
        
        VOID* region1;
        DWORD sizeRegion1;
        VOID* region2;
        DWORD sizeRegion2;
        SecondaryBuffer->Lock(BytesToLock, BytesToWrite, &region1, &sizeRegion1, &region2, &sizeRegion2, 0);

        FillSoundRegion(region1, sizeRegion1, sound_data);
        FillSoundRegion(region2, sizeRegion2, sound_data);

        SecondaryBuffer->Unlock(region1, sizeRegion1, region2, sizeRegion2);
    }
}

LRESULT CALLBACK
Win32MainWindowCallback( HWND   Window,
                    UINT   Message,
                    WPARAM WParam,
                    LPARAM LParam
)
{
    LRESULT result = 0;

    switch(Message) {
        case WM_SIZE: {
            OutputDebugStringA("WM_SIZE\n");
        } break;

        case WM_PAINT: {
            OutputDebugStringA("WM_PAINT\n");
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(Window, &paint);            
            auto dimensions = win32GetWindowDimension(Window);
            Win32UpdateWindow(device_context, dimensions.first, dimensions.second, global_backbuffer, 0, 0);

            EndPaint(Window, &paint);
        } break;

        case WM_DESTROY: {
            OutputDebugStringA("WM_DESTROY\n");   
            running = false;         
        } break;

        case WM_CLOSE: {
            running = false;
            OutputDebugStringA("WM_CLOSE\n");            
        } break;

        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32_t VKCode = WParam;
            bool WasDown = (LParam & 1 << 30) != 0;
            bool IsDown = (LParam & 1 << 31) == 0;

            switch(VKCode) {
                case 'W': {
                } break;
                case 'A': {
                } break;
                case 'S': {
                } break;
                case 'D': {
                } break;
                case 'Q': {
                } break;
                case 'E': {
                } break;
                case VK_UP: {
                } break;
                case VK_DOWN: {
                } break;
                case VK_LEFT: {
                } break;
                case VK_RIGHT: {
                } break;
                case VK_SPACE: {
                } break;
                case VK_ESCAPE: {
                } break;
                default: break;
            }
        }

        default: {
            result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return result;
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowWindow)
{
    Win32InitXInput();
	Win32ResizeDeviceIndependentBitmap(&global_backbuffer, 1280, 720);

    // TODO(apirogov): win32 GetFileTime function needs a file handle to get last write time.
    // I wonder if fs::last_write_time does the same. For now, I'm assuming It works in some other way doesn't create HANDLE implicitly.
	auto last_write_time = fs::last_write_time("../libs/render.dll");
    bool result = ::CopyFile("../libs/render.dll", "render_temp.dll", false);
    result = ::CopyFile("../libs/render.pdb", "render_temp.pdb", false);
    result = ::CopyFile("../libs/vc140.pdb", "vc140_temp.pdb", false);
    HMODULE render_dll = ::LoadLibrary("render_temp.dll");
    auto init_impl = reinterpret_cast<decltype(init) *>(
        ::GetProcAddress(render_dll, NAME(init)));
    auto render_impl = reinterpret_cast<decltype(render) *>(
        ::GetProcAddress(render_dll, NAME(render)));

    WNDCLASS window_class = {};
    window_class.style = CS_OWNDC|CS_HREDRAW;
    window_class.lpfnWndProc = Win32MainWindowCallback;
    window_class.hInstance = Instance;
    window_class.lpszClassName = "Win32SoftrenderWindowClass";

    if(RegisterClass(&window_class)) {
        HWND window_handle = CreateWindowEx(0,
                                            window_class.lpszClassName,
                                            "Win32Softrender",
                                            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            0,
                                            0,
                                            Instance,
                                            0);
        if(window_handle){
            Win32InitDirectSound(window_handle, global_sound_data.SamplesPerSecond, global_sound_data.SecondaryBufferSize);

            running = true;
            MSG message;
            init_impl(reinterpret_cast<uint32_t *>(global_backbuffer.memory), global_backbuffer.width, global_backbuffer.height, global_backbuffer.pitch);
            while(running) {
				auto last_write_time_new = fs::last_write_time("../libs/render.dll");
				if(last_write_time_new > last_write_time) {
                    last_write_time = last_write_time_new;
                    auto free_lib_result = ::FreeLibrary(render_dll);
					render_dll = 0;
                    // TODO(apirogov): For some reason fs::copy_file has failed me with throwing exception instead of returning an error code
                    bool result = ::CopyFile("../libs/render.dll", "render_temp.dll", false);
                    result = ::CopyFile("../libs/render.pdb", "render_temp.pdb", false);
                    result = ::CopyFile("../libs/vc140.pdb", "vc140_temp.pdb", false);
                    render_dll = ::LoadLibrary("render_temp.dll");
                    init_impl = reinterpret_cast<decltype(init) *>(
                        ::GetProcAddress(render_dll, NAME(init)));
                    render_impl = reinterpret_cast<decltype(render) *>(
                        ::GetProcAddress(render_dll, NAME(render)));
                    init_impl(reinterpret_cast<uint32_t *>(global_backbuffer.memory), global_backbuffer.width, global_backbuffer.height, global_backbuffer.pitch);
                }
                while(PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
                    if(message.message == WM_QUIT) {
                        running = false;
                    }
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                // TODO: Extract to separate system
                for (DWORD controllerIndex=0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++ )
                {
                    XINPUT_STATE controllerState;
                    if(DynamicXInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS) {
                        // NOTE: This controller is plugged in
                        XINPUT_GAMEPAD *gamepad = &controllerState.Gamepad;

                        bool dpadUp = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                        bool dpadDown = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                        bool dpadLeft = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                        bool dpadRight = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                        bool buttonStart = gamepad->wButtons & XINPUT_GAMEPAD_START;
                        bool buttonBack = gamepad->wButtons & XINPUT_GAMEPAD_BACK;
                        bool thumbLeft = gamepad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
                        bool thumbRight = gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
                        bool shoulderLeft = gamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                        bool shoulderRight = gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                        bool buttonA = gamepad->wButtons & XINPUT_GAMEPAD_A;
                        bool buttonB = gamepad->wButtons & XINPUT_GAMEPAD_B;
                        bool buttonX = gamepad->wButtons & XINPUT_GAMEPAD_X;
                        bool buttonY = gamepad->wButtons & XINPUT_GAMEPAD_Y;

                        int16_t leftStickX = gamepad->sThumbLX;
                        int16_t leftStickY = gamepad->sThumbLY;
                    } else {
                        // NOTE: The controller is not available
                    }
                }

                render_impl(0);

                // PlaySineWave(&global_sound_data);
                XINPUT_VIBRATION vibration;
                vibration.wLeftMotorSpeed = 6000;
                vibration.wRightMotorSpeed = 6000;
                DynamicXInputSetState(0, &vibration);

                HDC device_context = GetDC(window_handle);
                            
                auto dimensions = win32GetWindowDimension(window_handle);
                Win32UpdateWindow(device_context, dimensions.first, dimensions.second, global_backbuffer, 0, 0);
                ReleaseDC(window_handle, device_context);
            }
            
        } else {

        }
    } else {

    }
    
    return 0;
}