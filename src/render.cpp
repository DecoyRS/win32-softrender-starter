#include "render.h"

#include "camera.h"
#include "hitable.h"
#include "ray.h"
#include "sphere.h"

#include "lambertian.hpp"
#include "metal.hpp"
#include "dielectric.hpp"

#include "math/float3.h"
#include "utility.hpp"

#include <chrono>
#include <cstdint>
#include <float.h>
#include <iostream>
#include <optional>
#include <vector>
#include <thread>

namespace
{
    static int32_t last_segment = 0;
    static int32_t number_of_segments = 144;
    static int32_t segment_height = 0;
    static constexpr std::chrono::milliseconds max_delta_time{50};
    static constexpr uint32_t NUMBER_OF_PICKS = 100;

    const float R = cos(M_PI/4);

    static std::vector<Hitable *> generate_random_scene() {
        constexpr int n = 500;
        std::vector<Hitable*> temp;
        temp.reserve(n);
        temp.emplace_back(new Sphere(float3(0,-1000,0), 1000, new Lambertian(float3(0.5, 0.5, 0.5))));
        int i = 1;
        for (int a = -11; a < 11; a++) {
            for (int b = -11; b < 11; b++) {
                float choose_mat = dist(mt);
                float3 center(a+0.9*dist(mt),0.2,b+0.9*dist(mt)); 
                if (length(center-float3(4,0.2,0)) > 0.9) { 
                    if (choose_mat < 0.8) {  // diffuse
                        temp.emplace_back(new Sphere(center, 0.2, new Lambertian(float3(dist(mt)*dist(mt), dist(mt)*dist(mt), dist(mt)*dist(mt)))));
                    }
                    else if (choose_mat < 0.95) { // metal
                        temp.emplace_back(new Sphere(center, 0.2,
                                new Metal(float3(0.5*(1 + dist(mt)), 0.5*(1 + dist(mt)), 0.5*(1 + dist(mt))),  0.5*dist(mt))));
                    }
                    else {  // glass
                        temp.emplace_back(new Sphere(center, 0.2, new Dielectric(1.5)));
                    }
                }
            }
        }

        temp.emplace_back(new Sphere(float3(0, 1, 0), 1.0, new Dielectric(1.5)));
        temp.emplace_back(new Sphere(float3(-4, 1, 0), 1.0, new Lambertian(float3(0.4, 0.2, 0.1))));
        temp.emplace_back(new Sphere(float3(4, 1, 0), 1.0, new Metal(float3(0.7, 0.6, 0.5), 0.0)));
        return (temp);
    }

    static const std::vector<Hitable *> hitables = generate_random_scene();
    // {
    //     // new Sphere(float3(-R, 0, -1), R, new Lambertian(float3(0, 0, 1))),
    //     // new Sphere(float3(R, 0, -1), R, new Lambertian(float3(1, 0, 0)))

    //     new Sphere(float3(0,0,-1), 0.5, new Lambertian(float3(0.8, 0.3, 0.3))),
    //     new Sphere(float3(0, -100.5, -1), 100, new Lambertian(float3(0.8, 0.8, 0.0))),
    //     new Sphere(float3(1, 0, -1), 0.5, new Metal(float3(0.8, 0.6, 0.2), 0.3f)),
    //     new Sphere(float3(-1, 0, -1), 0.5, new Dielectric(1.5f)),
    //     new Sphere(float3(-1, 0, -1), -0.45, new Dielectric(1.5f))
    //     // new Sphere(float3(-1, 0, -1), 0.5, new Metal(float3(0.8, 0.8, 0.8), 1.f))
    // };
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

float3 color(const Ray& ray, const std::vector<Hitable*>& objects, int32_t depth) {
    if(auto hit_record = hit_list(ray, objects, 0.001f, FLT_MAX); hit_record.has_value() ) {
        auto scatter_result = hit_record.value().mat->scatter(ray, hit_record.value());
        if(depth < 50 && scatter_result.has_value())
            return scatter_result.value().second * color(scatter_result.value().first, objects, depth + 1);
        else
            return float3(0,0,0);
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
static float ASPECT_RATIO;

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
    ASPECT_RATIO = float(WIDTH)/HEIGHT;

    segment_height = HEIGHT / number_of_segments;

    fill_checkerboard(buffer, width, height, pitch);
}

void render_job(const uint32_t, int slice);

void __cdecl render(const uint32_t delta_time) {
    auto start_time = std::chrono::high_resolution_clock::now();
    while(true) {
        // ---
        std::vector<std::thread> threads;
        for(int i = 0; i < std::thread::hardware_concurrency(); i++, ++last_segment) {
            last_segment %= (number_of_segments + 1);
            threads.push_back(std::thread(render_job, delta_time, last_segment));
        }
        for(auto& t : threads)
            t.join();
        // ---        
        auto end_time = std::chrono::high_resolution_clock::now();
        if(end_time - start_time > max_delta_time)
            return;
        start_time = end_time;
    }
}

void render_job(const uint32_t delta_time, int slice) {
    if(!BUFFER)
        return;

    const int ROWS_PER_THREAD = segment_height;
    uint8_t *start_row = reinterpret_cast<uint8_t *>(BUFFER);
    uint8_t *current_row = start_row + PITCH*ROWS_PER_THREAD*slice;
    const float3 look_from{13, 2, 3};
    const float3 look_at{0, 0, 0};
    float distance_to_focus = 10.0f; // length(look_from - look_at);
    float aperture = 0.1f;

    Camera camera(look_from, look_at, float3(0, 1, 0), 20, ASPECT_RATIO, aperture, distance_to_focus);
    for (uint32_t last_row = ROWS_PER_THREAD*slice; last_row < (ROWS_PER_THREAD*(slice+1)) && last_row < HEIGHT; ++last_row) {
        uint32_t * pixel = (uint32_t *) current_row;
        for (uint32_t last_col = 0; last_col < WIDTH; ++last_col) {
            // if(last_col == 0 || last_row == ROWS_PER_THREAD*slice || last_col == (WIDTH - 1) || last_row == (ROWS_PER_THREAD*(slice+1) - 1)) {    
            //     const uint32_t ir = 0;
            //     const uint32_t ig = 255;
            //     const uint32_t ib = 0;
            //     *pixel++ = (ir << 16) | (ig << 8) | (ib);
            //     continue;
            // }
            float3 last_color{0, 0, 0};
            for(uint32_t last_ns = 0; last_ns < NUMBER_OF_PICKS; last_ns++) {    
                const float u = float(last_col + dist(mt)) / float(WIDTH);
                const float v = float(last_row + dist(mt)) / float(HEIGHT);
                Ray ray = camera.get_ray(u, v);
                float3 point = ray.point_at_parameter(2.0);
                last_color += color(ray, hitables, 0);
            }
            last_color /= float(NUMBER_OF_PICKS);
            last_color = float3(sqrt(last_color.r), sqrt(last_color.g), sqrt(last_color.b));
            const uint32_t ir = uint32_t(255.99*last_color.r);
            const uint32_t ig = uint32_t(255.99*last_color.g);
            const uint32_t ib = uint32_t(255.99*last_color.b);
            *pixel++ = (ir << 16) | (ig << 8) | (ib);
            last_color = {0, 0, 0};
        }
        current_row += PITCH;
    }
}