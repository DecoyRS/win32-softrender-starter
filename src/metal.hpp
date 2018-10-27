#pragma once
#include "material.hpp"

#include "utility.hpp"

class Metal : public Material {
public:
    Metal(float3 _albedo, float _fuzz): albedo(_albedo), fuzz(_fuzz < 1.f ? _fuzz : 1.f) {}
    std::optional<std::pair<Ray, float3>> scatter (const Ray& ray, const HitRecord& hit_record) const {
        float3 reflected = reflect(normalize(ray.direction()), hit_record.normal);
        Ray scattered(hit_record.p, reflected + fuzz*random_in_unit_sphere());
        float3 attenuation = albedo;
        return std::make_pair(scattered, attenuation);
    }

private:
    float3 albedo;
    float fuzz;

};