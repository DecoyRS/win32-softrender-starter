#pragma once
#include "math/float3.h"

#include <random>
#include <optional>

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_real_distribution<float> dist(0.0, 1.0);

float3 random_in_unit_sphere() {
    float3 p;
    do {
        p = 2.0f * float3(dist(mt), dist(mt), dist(mt)) - float3(1, 1, 1);
    } while (lengthSq(p) >= 1.0f);
    return p;
}


float3 reflect(float3 v, float3 n){
    return v - 2*dot(v, n) * n;
}

float schlick(float cosine, float ref_idx) {
    float r0 = (1 - ref_idx) / (1 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1 - r0)*pow((1 - cosine), 5);
}

std::optional<float3> refract(const float3 v, const float3 n, float ni_over_nt) {
    float3 unit_v = normalize(v);
    float dt = dot(unit_v, n);
    float discriminant = 1.0 - ni_over_nt*ni_over_nt*(1 - dt*dt);
    if(discriminant > 0)
        return ni_over_nt*(unit_v - n*dt) - n*sqrt(discriminant);
    return {};
}

float3 random_in_unit_disk() {
    float3 point;
    do {
        point = 2.0 * float3(dist(mt), dist(mt), 0) - float3(1, 1, 0);
    } while(dot(point, point) >= 1.0f);
    return point;
}