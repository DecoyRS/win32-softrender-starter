#pragma once

#include "material.hpp"

#include "math/float3.h"
#include "hitable.h"

class Sphere : public Hitable {
public:
    Sphere() {}
    Sphere(float3 _center, float _radius, Material * _mat) : center(_center), radius(_radius), mat(_mat) {}
    virtual std::optional<HitRecord> hit(const Ray & ray, float t_min, float t_max) const override;
    
    float3 center;
    float radius;
    Material * mat;
};

std::optional<HitRecord> Sphere::hit(const Ray & ray, float t_min, float t_max) const {
    float3 originToCenter = ray.origin() - center;
    float a = dot(ray.direction(), ray.direction());
    float b = dot(originToCenter, ray.direction());
    float c = dot(originToCenter, originToCenter) - radius*radius;
    float discriminant = b*b - a*c;
    if(discriminant > 0) {
        float sqrt_discriminant = sqrt(discriminant);
        float root = (-b - sqrt_discriminant)/a;
        if(t_max > root && root > t_min)
            return {HitRecord(root, ray.point_at_parameter(root), (ray.point_at_parameter(root) - center) / radius, mat)};
        root = (-b + sqrt_discriminant)/a;
        if(t_max > root && root > t_min)
            return {HitRecord(root, ray.point_at_parameter(root), (ray.point_at_parameter(root) - center) / radius, mat)};
    }
    return {};
}