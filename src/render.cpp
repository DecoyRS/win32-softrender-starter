#include "render.h"

#include "camera.h"
#include "hitable.h"
#include "ray.h"
#include "sphere.h"

#include "math/float3.h"

#include <chrono>
#include <cstdint>
#include <float.h>
#include <iostream>
#include <optional>
#include <random>
#include <vector>

namespace
{
    static uint32_t last_row = 0;
    static uint32_t last_col = 0;
    static uint32_t last_ns = 0;
    static float3 last_color{0, 0, 0};

    static constexpr std::chrono::milliseconds max_delta_time{100};
    static constexpr uint32_t NUMBER_OF_PICKS = 10;

    static const std::vector<Hitable *> hitables = {
        new Sphere(float3(0,0,-1), 0.5),
        new Sphere(float3(0, -100.5, -1), 100)
    };

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(0.0, 1.0);
}

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

float3 random_in_init_sphere() {
    float3 p;
    do {
        p = 2.0f * float3(dist(mt), dist(mt), dist(mt)) - float3(1, 1, 1);
    } while (lengthSq(p) >= 1.0f);
    return p;
}

float3 color(const Ray& ray, const std::vector<Hitable*>& objects) {
    if(auto hit_record = hit_list(ray, objects, 0.001f, FLT_MAX); hit_record.has_value() ) {
        float3 target = hit_record.value().p + hit_record.value().normal + random_in_init_sphere();
        return 0.5f * color(Ray(hit_record.value().p, target - hit_record.value().p), objects);
        // return 0.5f*float3(hit_record.value().normal.x + 1,
        //                    hit_record.value().normal.y + 1,
        //                    hit_record.value().normal.z + 1);
    } else {
        float3 unit_direction = normalize(ray.direction());
        float t = 0.5f*(unit_direction.y + 1.0f);
        return (1.0f - t) * float3(1.0f , 1.0f, 1.0f) + t*float3(0.5, 0.7, 1.0); 
    }
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


void __cdecl render(const uint32_t delta_time) {
    if(!BUFFER)
        return;

    const auto start_time = std::chrono::high_resolution_clock::now();

    uint8_t *start_row = reinterpret_cast<uint8_t *>(BUFFER);
    uint8_t *current_row = start_row + PITCH*last_row;
    Camera camera;
    for (; last_row < HEIGHT; ++last_row) {
        uint32_t * pixel = (uint32_t *) current_row;
        pixel += last_col;
        for (; last_col < WIDTH; ++last_col) {
            for(; last_ns < NUMBER_OF_PICKS; last_ns++) {    
                const float u = float(last_col + dist(mt)) / float(WIDTH);
                const float v = float(last_row + dist(mt)) / float(HEIGHT);
                Ray ray = camera.get_ray(u, v);
                float3 point = ray.point_at_parameter(2.0);
                last_color += color(ray, hitables);
				const auto delta_time = std::chrono::high_resolution_clock::now() - start_time;
                if(delta_time > max_delta_time) {
                    return;
                }
            }
            last_ns = 0;
            last_color /= float(NUMBER_OF_PICKS);
            last_color = float3(sqrt(last_color.r), sqrt(last_color.g), sqrt(last_color.b));
            const uint32_t ir = uint32_t(255.99*last_color.r);
            const uint32_t ig = uint32_t(255.99*last_color.g);
            const uint32_t ib = uint32_t(255.99*last_color.b);
            *pixel++ = (ir << 16) | (ig << 8) | (ib);
            last_color = {0, 0, 0};
        }
        last_col = 0;
        current_row += PITCH;
    }
    last_row = 0;
}