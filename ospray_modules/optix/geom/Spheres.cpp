// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#undef NDEBUG

#include <ospray/SDK/common/Data.h>

#include "../Model.h"
#include "../render/Material.h"
#include "Spheres.h"

namespace bbp
{
namespace optix
{
std::string Spheres::toString() const
{
    return "optix::Spheres";
}

void Spheres::finalize(Model* optixModel)
{
    ospray::Ref<ospray::Data> data = getParamData("spheres", nullptr);
    if (!data.ptr)
        throw std::runtime_error("#optix:geometry/spheres: "
                                 "no 'spheres' data specified");

    Geometry::finalize(optixModel);

    const size_t bytesPerSphere =
        getParam1i("bytes_per_sphere", 7 * sizeof(float));
    _context["bytes_per_sphere"]->setUint(bytesPerSphere);
    _geometry->setPrimitiveCount(data->numBytes / bytesPerSphere);

    _setBuffer("spheres", data);
}
}
}
