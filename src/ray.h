#pragma once

#include "math/float3.h"

class Ray {
public:
    Ray() {}
    Ray(float3 a, float3 b):A(a), B(b){}

    float3 origin() const { return A; }
    float3 direction() const { return B; }
    float3 point_at_parameter(float dt) const { return A + dt*B; }    

    float3 A;
    float3 B;    
};