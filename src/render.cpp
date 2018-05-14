#include "render.h"

static uint32_t * BUFFER;
static uint32_t WIDTH;
static uint32_t HEIGHT;
static uint32_t PITCH;

void init(uint32_t * buffer, const uint32_t width, const uint32_t height, const uint32_t pitch) {
    BUFFER = buffer;
    WIDTH = width;
    HEIGHT = height;
    PITCH = pitch;
}

void render(const uint32_t delta_time) {
    if(!BUFFER)
        return;
    uint8_t *row = reinterpret_cast<uint8_t *>(BUFFER);
    for (uint32_t y = 0; y < HEIGHT; ++y)
    {
        uint32_t * pixel = (uint32_t *) row;
        for (int x = 0; x < WIDTH; ++x)
        {
            const float r = float(x) / float(WIDTH);
            const float g = float(y) / float(HEIGHT);
            const float b = 0.2;
            const uint32_t ir = uint32_t(255.99*r);
            const uint32_t ig = uint32_t(255.99*g);
            const uint32_t ib = uint32_t(255.99*b);
            *pixel++ = (ir << 16) | (ig << 8) | (ib);
        }
        row += PITCH;
    }
}