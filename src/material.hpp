#pragma once
#include "ray.h"
#include "hitable.h"

#include <optional>
#include <utility>

class Material {
public:
    virtual std::optional<std::pair<Ray, float3>> scatter (const Ray& ray, const HitRecord& hit_record) const = 0;
};