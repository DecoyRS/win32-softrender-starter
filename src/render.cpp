#include "render.h"

#include "ray.h"
#include "camera.h"

#include "math/float3.h"

#include <cstdint>
#include <chrono>
#include <random>
#include <iostream>

float hit_sphere(float3 center, float radius, const Ray& r) {
    float3 oc = r.origin() - center;
    float a = dot(r.direction(), r.direction());
    float b = 2.0 * dot(oc, r.direction());
    float c = dot(oc, oc) - radius*radius;
    float discriminant = b*b - 4*a*c;
    if (discriminant < 0) {
        return -1.0;
    } else {
        return (-b - sqrt(discriminant) ) / (2.0*a);
    }
}

float3 color(const Ray& r){
    float t = hit_sphere(float3(0.0, 0.0, -1.0), 0.5, r);
    if ( t > 0.0) {
        float3 normal = normalize(r.point_at_parameter(t) - float3(0, 0, -1));
        return 0.5*float3(normal.x + 1, normal.y + 1, normal.z + 1);
    }
    float3 unit_direction = normalize(r.direction());
    t = 0.5*(unit_direction.y + 1.0);
    return (1.0 - t) * float3(1.0, 1.0, 1.0) + t*float3(0.5, 0.7, 1.0);
}

static uint32_t * BUFFER;
static uint32_t WIDTH;
static uint32_t HEIGHT;
static uint32_t PITCH;

void fill_checkerboard(uint32_t * buffer, const uint32_t width, const uint32_t height, const uint32_t pitch, const uint32_t start_row = 0, const uint32_t start_col = 0) {
    constexpr uint32_t cell_side = 40;    
    uint8_t *current_row = reinterpret_cast<uint8_t *>(buffer);
    for (uint32_t row = start_row; row < height; ++row) {
        uint32_t * pixel = (uint32_t *) current_row;
        for(uint32_t col = start_col; col < width; ++col) {
            const bool is_white_cell = (col / cell_side % 2 == 0) !=  (row / cell_side % 2 == 0);
            constexpr uint32_t dark_grey = (0xff << 24) | (191 << 16) | (191 << 8) | 191;
            constexpr uint32_t light_grey = (0xff << 24) | (200 << 16) | (200 << 8) | (200) ;
            *pixel++ = is_white_cell ? light_grey : dark_grey;
        }
        current_row += pitch;
    }
}

void __cdecl init(uint32_t * buffer, const uint32_t width, const uint32_t height, const uint32_t pitch) {
    BUFFER = buffer;
    WIDTH = width;
    HEIGHT = height;
    PITCH = pitch;

    fill_checkerboard(buffer, width, height, pitch);
}
static bool toggle = false;

static uint32_t last_row = 0;
static uint32_t last_col = 0;
static uint32_t last_ns = 0;
static float3 last_color{0, 0, 0};

static constexpr std::chrono::milliseconds max_delta_time{100};

void __cdecl render(const uint32_t delta_time) {
    if(!BUFFER)
        return;
    const float3 lower_left_corner(-2.0, -1.0, -1.0);
    const float3 horizontal(4.0, 0.0, 0.0);
    const float3 vertical(0.0, 2.0, 0.0);
    const float3 origin(0.0, 0.0, 0.0);
    constexpr uint32_t ns = 200;
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(0.0, 1.0);

    const auto start_time = std::chrono::high_resolution_clock::now();

    uint8_t *start_row = reinterpret_cast<uint8_t *>(BUFFER);
    uint8_t *current_row = start_row + PITCH*last_row;
    Camera camera;
    for (; last_row < HEIGHT; ++last_row) {
        uint32_t * pixel = (uint32_t *) current_row;
        pixel += last_col;
        for (; last_col < WIDTH; ++last_col) {
            last_color = {0, 0, 0};
            for(; last_ns < ns; last_ns++) {    
                const float u = float(last_col + dist(mt)) / float(WIDTH);
                const float v = float(last_row + dist(mt)) / float(HEIGHT);
                Ray ray = camera.get_ray(u, v);
                float3 point = ray.point_at_parameter(2.0);
                last_color += color(ray);
				const auto delta_time = std::chrono::high_resolution_clock::now() - start_time;
                if(delta_time > max_delta_time) {
                    return;
                }
            }
            last_ns = 0;
            last_color /= float(ns);
            const uint32_t ir = uint32_t(255.99*last_color.r);
            const uint32_t ig = uint32_t(255.99*last_color.g);
            const uint32_t ib = uint32_t(255.99*last_color.b);
            *pixel++ = (ir << 16) | (ig << 8) | (ib);
        }
        last_col = 0;
        current_row += PITCH;
    }
    last_row = 0;
}