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

#include <brayns/engine/Scene.h>

#include "OSPRayCamera.h"
#include "utils.h"

namespace brayns
{
OSPRayCamera::~OSPRayCamera()
{
    ospRelease(_camera);
}

void OSPRayCamera::commit()
{
    if (!isModified())
        return;

    const bool cameraChanged = _currentOSPCamera != getCurrentType();
    if (cameraChanged)
        _createOSPCamera();

    const auto& position = getPosition();
    const auto& dir = getOrientation().rotate(Vector3f(0.0f, 0.0f, -1.0f));
    const auto& up = getOrientation().rotate(Vector3f(0.0f, 1.0f, 0.0f));

    ospSet3f(_camera, "pos", position.x(), position.y(), position.z());
    ospSet3f(_camera, "dir", dir.x(), dir.y(), dir.z());
    ospSet3f(_camera, "up", up.x(), up.y(), up.z());
    ospSetString(_camera, "buffer_target", getBufferTarget().c_str());

    toOSPRayProperties(*this, _camera);

    // Clip planes
    if (!_clipPlanes.empty())
    {
        const auto clipPlanes = convertVectorToFloat(_clipPlanes);
        auto clipPlaneData =
            ospNewData(clipPlanes.size(), OSP_FLOAT4, clipPlanes.data());
        ospSetData(_camera, "clipPlanes", clipPlaneData);
        ospRelease(clipPlaneData);
    }
    else
    {
        // ospRemoveParam leaks objects, so we set it to null first
        ospSetData(_camera, "clipPlanes", nullptr);
        ospRemoveParam(_camera, "clipPlanes");
    }

    ospCommit(_camera);
}

void OSPRayCamera::setEnvironmentMap(const bool environmentMap)
{
    ospSet1i(_camera, "environmentMap", environmentMap);
    ospCommit(_camera);
}

void OSPRayCamera::setClipPlanes(const Planes& planes)
{
    if (_clipPlanes == planes)
        return;
    _clipPlanes = planes;
    markModified(false);
}

void OSPRayCamera::_createOSPCamera()
{
    auto newCamera = ospNewCamera(getCurrentType().c_str());
    if (!newCamera)
        throw std::runtime_error(getCurrentType() +
                                 " is not a registered camera");
    if (_camera)
        ospRelease(_camera);
    _camera = newCamera;
    _currentOSPCamera = getCurrentType();
    markModified();
}
}
