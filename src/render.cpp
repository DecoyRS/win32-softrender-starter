#include "render.h"

#include "math/float3.h"

static uint32_t * BUFFER;
static uint32_t WIDTH;
static uint32_t HEIGHT;
static uint32_t PITCH;

void __cdecl init(uint32_t * buffer, const uint32_t width, const uint32_t height, const uint32_t pitch) {
    BUFFER = buffer;
    WIDTH = width;
    HEIGHT = height;
    PITCH = pitch;
}

void __cdecl render(const uint32_t delta_time) {
    if(!BUFFER)
        return;
    uint8_t *row = reinterpret_cast<uint8_t *>(BUFFER);
    for (uint32_t y = 0; y < HEIGHT; ++y)
    {
        uint32_t * pixel = (uint32_t *) row;
        for (int x = 0; x < WIDTH; ++x)
        {
            float3 color(float(x) / float(WIDTH), float(y) / float(HEIGHT), 0.2);
            const uint32_t ir = delta_time - uint32_t(255.99*color.r);
            const uint32_t ig = delta_time - uint32_t(255.99*color.g);
            const uint32_t ib = delta_time - uint32_t(255.99*color.b);
            *pixel++ = (ir << 16) | (ig << 8) | (ib);
        }
        row += PITCH;
    }
}