/* Copyright (c) 2017, EPFL/Blue Brain Project
 * All rights reserved. Do not distribute without permission.
 * Responsible Author: Daniel Nachbaur <daniel.nachbaur@epfl.ch>
 *
 * This file is part of https://github.com/BlueBrain/ospray-modules
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

#pragma once

#include <ospray/SDK/common/Data.h>

#include <optixu/optixpp_namespace.h>

#include <memory>
#include <mutex>
#include <unordered_map>

#include "geom/Geometry.h"

namespace bbp
{
namespace optix
{

class Geometry;
class Texture;

class Context
{
public:
    ~Context();
    static Context& get();
    static void destroy();

    ::optix::Context getOptixContext();
    ::optix::Material createMaterial(bool textured);
    ::optix::TextureSampler createTexture(Texture* tx);
    void deleteTexture(Texture* tx);
    ::optix::TextureSampler getTextureSampler(Texture* tx);
    void updateLights(const ospray::Data* lightData = nullptr);

    ::optix::Geometry createGeometry(Geometry::Type type);

    std::unique_lock<std::mutex> getScopeLock()
    {
        return std::unique_lock<std::mutex>(_mutex);
    }

private:
    Context();

    void _initialize();
    void _printSystemInformation() const;

    static std::unique_ptr<Context> _context;

    ::optix::Context _optixContext;
    ::optix::Program _phong_ah;
    ::optix::Program _phong_ch;
    ::optix::Program _phong_ch_textured;
    std::array<::optix::Program, Geometry::Type::SIZE> _bounds;
    std::array<::optix::Program, Geometry::Type::SIZE> _intersects;

    const ospray::Data* _lightData = nullptr;
    ::optix::Buffer _lightBuffer;

    std::unordered_map<void*, ::optix::TextureSampler> _optixTextureSamplers;
    std::mutex _mutex;
};
}
}
