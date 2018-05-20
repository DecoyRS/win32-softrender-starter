#include "render.h"

#include "math/float3.h"

#include "ray.h"

float3 color(const ray& r){
    float3 unit_direction = normalize(r.direction());
    float t = 0.5*(unit_direction.y + 1.0);
    return (1.0 - t) * float3(1.0, 1.0, 1.0) + t*float3(0.5, 0.7, 1.0);
}

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
    const float3 lower_left_corner(-2.0, -1.0, -1.0);
    const float3 horizontal(4.0, 0.0, 0.0);
    const float3 vertical(0.0, 2.0, 0.0);
    const float3 origin(0.0, 0.0, 0.0);
    uint8_t *row = reinterpret_cast<uint8_t *>(BUFFER);
    for (uint32_t y = 0; y < HEIGHT; ++y)
    {
        uint32_t * pixel = (uint32_t *) row;
        for (int x = 0; x < WIDTH; ++x)
        {
            const float u = float(x) / float(WIDTH);
            const float v = float(y) / float(HEIGHT);
            const ray r(origin, lower_left_corner + u*horizontal + v*vertical);
            const float3 col = color(r);
            const uint32_t ir = uint32_t(255.99*col.r);
            const uint32_t ig = uint32_t(255.99*col.g);
            const uint32_t ib = uint32_t(255.99*col.b);
            *pixel++ = (ir << 16) | (ig << 8) | (ib);
        }
        row += PITCH;
    }
}