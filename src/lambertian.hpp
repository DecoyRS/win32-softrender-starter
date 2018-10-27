#pragma once
#include "material.hpp"

#include "utility.hpp"

class Lambertian : public Material {

public:
    Lambertian(float3 _albedo): albedo(_albedo){}

    virtual std::optional<std::pair<Ray, float3>> scatter (const Ray& ray, const HitRecord& hit_record) const {
        float3 target = hit_record.p + hit_record.normal + random_in_unit_sphere();
        Ray ray_out(hit_record.p, target - hit_record.p);
        float3 attenuation = albedo;
        return std::make_pair(ray_out, attenuation);
    }

    float3 albedo;
};