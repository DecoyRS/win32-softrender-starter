#pragma once

#include "ray.h"

#include "math/float3.h"

#include <optional>
#include <vector>

class Material;

struct HitRecord {
    HitRecord(float _t, float3 _p, float3 _n, Material * _mat): t(_t), p(_p), normal(_n), mat(_mat) {}
    HitRecord(const HitRecord&) = default;
    float t;
    float3 p;
    float3 normal;
    Material * mat;
};

class Hitable {
public:
    virtual std::optional<HitRecord> hit(const Ray & ray, float t_min, float t_max ) const = 0;
};

std::optional<HitRecord> hit_list(const Ray& ray, const std::vector<Hitable*> & world, float t_min, float t_max) {
    std::optional<HitRecord> result;
    double closest_so_far = t_max;
    for(auto item : world) {
        if(auto res = item->hit(ray, t_min, closest_so_far); res.has_value()) {
            closest_so_far = res.value().t;
            result = res;
        }
    }
    return result;
}