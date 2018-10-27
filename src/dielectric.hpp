#pragma once

#include "material.hpp"

#include "utility.hpp"

class Dielectric : public Material {
public:
    Dielectric(float ri): ref_idx(ri){}

    std::optional< std::pair<Ray, float3> > scatter (const Ray& ray, const HitRecord& hit_record) const {
        float3 reflected = reflect(ray.direction(), hit_record.normal);
        float3 attenuation = float3(1.f, 1.f, 1.f);

        float3 outward_normal;
        float ni_over_nt;
        float cosine;
        if(dot(ray.direction(), hit_record.normal) > 0) {
            outward_normal = -hit_record.normal;
            ni_over_nt = ref_idx;
            cosine = ref_idx * dot(ray.direction(), hit_record.normal) / length(ray.direction());
        } else {
            outward_normal = hit_record.normal;
            ni_over_nt = 1.f / ref_idx;
            cosine = -dot(ray.direction(), hit_record.normal) / length(ray.direction());
        }
        float reflect_prob;
        auto refracted = refract(ray.direction(), outward_normal, ni_over_nt);
        if( refracted.has_value()) {
            reflect_prob = schlick(cosine, ref_idx);
            if(dist(mt) > reflect_prob)
                return std::make_pair(Ray(hit_record.p, refracted.value()), attenuation);
        }
        return std::make_pair(Ray(hit_record.p, reflected), attenuation);        
    }

private:
    float ref_idx;
};