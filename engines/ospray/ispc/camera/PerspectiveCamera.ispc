// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "utils.ih"

#include "math/sampling.ih"
#include "ospray/SDK/camera/Camera.ih"

struct PerspectiveCamera
{
    Camera super;

    vec3f org;    //!< position of camera, already contains shift when
                  //! STEREO_{LEFT|RIGHT}
    vec3f dir_00; //!< direction of ray with screenSample=(0,0); scaled to
                  //! focusDistance
    vec3f dir_du; //!< delta of ray direction between two pixels in X; scaled to
                  //! focusDistance
    vec3f dir_dv; //!< delta of ray direction between two pixels in Y; scaled to
                  //! focusDistance
    float scaledAperture; //!< radius of aperture, divided by horizontal image
                          //! plane size
    float aspect;         //!< image plane size x / y
    vec3f ipd_offset;     //!< shift of camera position for left/right eye (only
                          //! when stereo)

    // Clip planes
    const uniform vec4f* clipPlanes;
    unsigned int numClipPlanes;
};

void PerspectiveCamera_initRay(uniform Camera* uniform _self, varying Ray& ray,
                               const varying CameraSample& sample)
{
    uniform PerspectiveCamera* uniform self =
        (uniform PerspectiveCamera * uniform)_self;

    vec2f screen = sample.screen;
    vec3f org = self->org;

    screen = Camera_subRegion(_self, screen);

    vec3f dir =
        self->dir_00 + screen.x * self->dir_du + screen.y * self->dir_dv;

    if (self->super.doesDOF)
    {
        const vec3f llp = uniformSampleDisk(self->scaledAperture, sample.lens);
        // transform local lens point to focal plane (dir_XX are prescaled in
        // this case)
        const vec3f lp =
            (llp.x * self->dir_du) + ((llp.y * self->aspect) * self->dir_dv);
        org = org + lp;
        dir = dir - lp;
    }

    float nearClip = self->super.nearClip;
    float farClip = infinity;
    const varying vec3f direction = normalize(dir);

    clipRay(self->clipPlanes, self->numClipPlanes, org, direction, nearClip,
            farClip);

    const float time = Camera_shutterTime(_self, sample.time);
    setRay(ray, org, direction, nearClip, farClip, time);
}

/*! create a new ispc-side version of a PerspectiveCamera - with given
    pointer to the c-class version - and return both class pointer and
    pointer to internal data back via the supplied reference
    parameters */
export void* uniform PerspectiveCamera_create(void* uniform cppE)
{
    uniform PerspectiveCamera* uniform self =
        uniform new uniform PerspectiveCamera;
    self->super.cppEquivalent = cppE;
    self->super.initRay = PerspectiveCamera_initRay;
    self->super.doesDOF = false;
    self->numClipPlanes = 0;
    return self;
}

export void PerspectiveCamera_set(
    void* uniform _self, const uniform vec3f& org, const uniform vec3f& dir_00,
    const uniform vec3f& dir_du, const uniform vec3f& dir_dv,
    const uniform float scaledAperture, const uniform float aspect,
    const uniform vec3f& ipd_offset, const uniform vec4f clipPlanes[],
    const uniform unsigned int numClipPlanes)
{
    uniform PerspectiveCamera* uniform self =
        (uniform PerspectiveCamera * uniform)_self;
    self->org = org;
    self->dir_00 = dir_00;
    self->dir_du = dir_du;
    self->dir_dv = dir_dv;
    self->scaledAperture = scaledAperture;
    self->super.doesDOF = scaledAperture > 0.f;
    self->aspect = aspect;
    self->ipd_offset = ipd_offset;
    self->clipPlanes = clipPlanes;
    self->numClipPlanes = numClipPlanes;
}
