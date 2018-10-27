#pragma once

#include "math/float3.h"
#include "ray.h"
#include "utility.hpp"

class Camera {
public:
    Camera(float3 look_from, float3 look_at, float3 up_vector, float vertical_fov, float aspect_ratio, float aperture, float focus_distance):
        origin{0.0, 0.0, 0.0}
    {
        lense_radius = aperture / 2;
        float theta = vertical_fov * M_PI / 180;
        float half_height = tan(theta/2);
        float half_width = aspect_ratio * half_height;
        origin = look_from;
        w = normalize(look_from - look_at);
        u = normalize(cross(up_vector, w));
        v = cross(w, u);
        // lower_left_corner = float3(-half_width, -half_height, -1.0f);
        lower_left_corner = origin - half_width*focus_distance*u - half_height*focus_distance*v - w*focus_distance;
        horizontal = 2*half_width*focus_distance*u;
        vertical = 2*half_height*focus_distance*v;
    }

    Ray get_ray(float s, float t) {
        float3 radius = lense_radius*random_in_unit_disk();
        float3 offset = u * radius.x + v*radius.y;
        return Ray(origin + offset, lower_left_corner + s*horizontal + t*vertical - origin - offset);
    }

    float3 origin;
    float3 lower_left_corner;
    float3 horizontal;
    float3 vertical;
    float3 u, v, w;
    float lense_radius;

};