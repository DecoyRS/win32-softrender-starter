#pragma once
#include <cstdint>

#ifdef DLL
    #define DLLEXPORT extern "C" __declspec(dllexport) 
#else
    #define DLLEXPORT
#endif

DLLEXPORT void init(uint32_t * buffer, const uint32_t width, const uint32_t height, const uint32_t pitch);
DLLEXPORT void render(const uint32_t delta_time);