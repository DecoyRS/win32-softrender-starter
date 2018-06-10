#pragma once

#include "math/float3.h"
#include "ray.h"

class Camera {
public:
    Camera():
        lower_left_corner{-2.0, -1.0, -1.0},
        horizontal{4.0, 0.0, 0.0},
        vertical{0.0, 2.0, 0.0},
        origin{0.0, 0.0, 0.0}
    {}

    Ray get_ray(float u, float v) { return Ray(origin, lower_left_corner + u*horizontal + v*vertical - origin); }

    float3 origin;
    float3 lower_left_corner;
    float3 horizontal;
    float3 vertical;
};