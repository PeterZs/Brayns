/* Copyright (c) 2015-2016, EPFL/Blue Brain Project
 * All rights reserved. Do not distribute without permission.
 * Responsible Author: Cyrille Favreau <cyrille.favreau@epfl.ch>
 *
 * This file is part of Brayns <https://github.com/BlueBrain/Brayns>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <optix_world.h>

using namespace optix;

// Global variables
rtBuffer<char> cylinders;

// Geometry specific variables
rtDeclareVariable(float3, geometric_normal, attribute geometric_normal, );
rtDeclareVariable(float3, shading_normal, attribute shading_normal, );
rtDeclareVariable(optix::Ray, ray, rtCurrentRay, );
rtDeclareVariable(unsigned int, bytes_per_cylinder, , );

__device__ inline const float* get_cylinder(const int primIdx)
{
    // We have to skip the first 8 bytes, which contain user data not used here
    return (float*)(&cylinders[primIdx * bytes_per_cylinder + 8]);
}

template <bool use_robust_method>
static __device__ void intersect_cylinder(int primIdx)
{
    const float* cylinder = get_cylinder(primIdx);
    const float3 v0 = {cylinder[0], cylinder[1], cylinder[2]};
    const float3 v1 = {cylinder[3], cylinder[4], cylinder[5]};
    const float radius = cylinder[6];

    const float3 A = v0 - ray.origin;
    const float3 B = v1 - ray.origin;

    const float3 O = make_float3(0.f);
    const float3 V = ray.direction;

    const float3 AB = B - A;
    const float3 AO = O - A;

    const float3 AOxAB = cross(AO, AB);
    const float3 VxAB = cross(V, AB);
    const float ab2 = dot(AB, AB);
    const float a = dot(VxAB, VxAB);
    const float b = 2.f * dot(VxAB, AOxAB);
    const float c = dot(AOxAB, AOxAB) - (radius * radius * ab2);

    const float radical = b * b - 4.f * a * c;
    if (radical >= 0.f)
    {
        // clip to near and far cap of cylinder
        const float tA = dot(AB, A) / dot(V, AB);
        const float tB = dot(AB, B) / dot(V, AB);
        // const float tAB0 = max( 0.f, min( tA, tB ));
        // const float tAB1 = min( RT_DEFAULT_MAX, max( tA, tB ));
        const float tAB0 = min(tA, tB);
        const float tAB1 = max(tA, tB);

        const float srad = sqrt(radical);

        const float t_in = (-b - srad) / (2.f * a);

        bool check_second = true;
        if (t_in >= tAB0 && t_in <= tAB1)
        {
            if (rtPotentialIntersection(t_in))
            {
                const float3 P = ray.origin + t_in * ray.direction - v0;
                const float3 V = cross(P, AB);
                geometric_normal = shading_normal = cross(AB, V);
                if (rtReportIntersection(0))
                    check_second = false;
            }
        }

        if (check_second)
        {
            const float t_out = (-b + srad) / (2.f * a);
            if (t_out >= tAB0 && t_out <= tAB1)
            {
                if (rtPotentialIntersection(t_out))
                {
                    const float3 P = t_out * ray.direction - A;
                    const float3 V = cross(P, AB);
                    geometric_normal = shading_normal = cross(AB, V);
                    rtReportIntersection(0);
                }
            }
        }
    }
}

RT_PROGRAM void intersect(int primIdx)
{
    intersect_cylinder<false>(primIdx);
}

RT_PROGRAM void robust_intersect(int primIdx)
{
    intersect_cylinder<true>(primIdx);
}

RT_PROGRAM void bounds(int primIdx, float result[6])
{
    const float* cylinder = get_cylinder(primIdx);
    const float3 v0 = {cylinder[0], cylinder[1], cylinder[2]};
    const float3 v1 = {cylinder[3], cylinder[4], cylinder[5]};
    const float radius = cylinder[6];

    optix::Aabb* aabb = (optix::Aabb*)result;

    if (radius > 0.f && !isinf(radius))
    {
        aabb->m_min = fminf(v0, v1) - radius;
        aabb->m_max = fmaxf(v0, v1) + radius;
    }
    else
        aabb->invalidate();
}
