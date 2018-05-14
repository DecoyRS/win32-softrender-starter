// TODO(apirogov): slim windows.h a little. Include only things that we need 

#include "render.h"

#include <windows.h>

#include <cstdint>
#include <filesystem>
#include <utility>

namespace fs = std::experimental::filesystem;

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

namespace {
    static const char * init_mangled = "init";
    static const char * render_mangled = "render";
}

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
                              const Win32OffscreenBuffer buffer,
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
	Win32ResizeDeviceIndependentBitmap(&global_backbuffer, 1280, 720);

    // TODO(apirogov): win32 GetFileTime function needs a file handle to get last write time.
    // I wonder if fs::last_write_time does the same. For now, I'm assuming It works in some other way doesn't create HANDLE implicitly.
	auto last_write_time = fs::last_write_time("../libs/render.dll");
    bool result = ::CopyFile("../libs/render.dll", "render_temp.dll", false);
    HMODULE render_dll = ::LoadLibrary("render_temp.dll");
    auto init_impl = reinterpret_cast<decltype(init) *>(
        ::GetProcAddress(render_dll, init_mangled));
    auto render_impl = reinterpret_cast<decltype(render) *>(
        ::GetProcAddress(render_dll, render_mangled));

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
            running = true;
            uint32_t x_offset = 0;
            uint32_t y_offset = 0;
            MSG message;
            init_impl(reinterpret_cast<uint32_t *>(global_backbuffer.memory), global_backbuffer.width, global_backbuffer.height, global_backbuffer.pitch);
            while(running) {
				auto last_write_time_new = fs::last_write_time("../libs/render.dll");
				if(last_write_time_new > last_write_time) {
                    last_write_time = last_write_time_new;
                    auto free_lib_result = ::FreeLibrary(render_dll);
					render_dll = 0;
                    // TODO(apirogov): For some reason fs::copy_file has failed me with throwing exception instead of returning an error code
                    auto copy_file_result = ::CopyFile("../libs/render.dll", "render_temp.dll", false);
                    render_dll = ::LoadLibrary("render_temp.dll");
                    init_impl = reinterpret_cast<decltype(init) *>(
                        ::GetProcAddress(render_dll, init_mangled));
                    render_impl = reinterpret_cast<decltype(render) *>(
                        ::GetProcAddress(render_dll, render_mangled));
                    init_impl(reinterpret_cast<uint32_t *>(global_backbuffer.memory), global_backbuffer.width, global_backbuffer.height, global_backbuffer.pitch);
                }
                while(PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
                    if(message.message == WM_QUIT) {
                        running = false;
                    }
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                render_impl(x_offset);

                HDC device_context = GetDC(window_handle);
                            
                auto dimensions = win32GetWindowDimension(window_handle);
                Win32UpdateWindow(device_context, dimensions.first, dimensions.second, global_backbuffer, 0, 0);
                ReleaseDC(window_handle, device_context);
                x_offset += 1;
                y_offset += 2;
            }
            
        } else {

        }
    } else {

    }
    
    return 0;
}