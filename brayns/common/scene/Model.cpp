/* Copyright (c) 2015-2018, EPFL/Blue Brain Project
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

#include "Model.h"

#include <brayns/common/Transformation.h>
#include <brayns/common/log.h>
#include <brayns/common/material/Material.h>
#include <brayns/common/material/Texture2D.h>

namespace brayns
{
ModelDescriptor::ModelDescriptor(const size_t id, const std::string& name,
                                 const std::string& path,
                                 const ModelMetadata& metadata, ModelPtr model)
    : _id(id)
    , _name(name)
    , _path(path)
    , _metadata(metadata)
    , _model(std::move(model))
{
}

bool Model::empty() const
{
    return _spheres.empty() && _cylinders.empty() && _cones.empty() &&
           _trianglesMeshes.empty() && _models.empty();
}

uint64_t Model::addSphere(const size_t materialId, const Sphere& sphere)
{
    _spheresDirty = true;
    _spheres[materialId].push_back(sphere);
    _bounds.merge(sphere.center);
    return _spheres[materialId].size() - 1;
}

uint64_t Model::addCylinder(const size_t materialId, const Cylinder& cylinder)
{
    _cylindersDirty = true;
    _cylinders[materialId].push_back(cylinder);
    _bounds.merge(cylinder.center);
    _bounds.merge(cylinder.up);
    return _cylinders[materialId].size() - 1;
}

uint64_t Model::addCone(const size_t materialId, const Cone& cone)
{
    _conesDirty = true;
    _cones[materialId].push_back(cone);
    _bounds.merge(cone.center);
    _bounds.merge(cone.up);
    return _cones[materialId].size() - 1;
}

void Model::addModel(ModelPtr model, const Transformations& transform)
{
    _bounds.merge(model->getBounds());
    _models.push_back({std::move(model), transform});
    _modelsDirty = true;
}

bool Model::dirty() const
{
    return _spheresDirty || _cylindersDirty || _conesDirty ||
           _trianglesMeshesDirty || _modelsDirty;
}

void Model::setMaterialsColorMap(const MaterialsColorMap colorMap)
{
    size_t index = 0;
    for (auto material : _materials)
    {
        material.second->setSpecularColor(Vector3f(0.f));
        material.second->setOpacity(1.f);
        material.second->setReflectionIndex(0.f);
        material.second->setEmission(0.f);

        switch (colorMap)
        {
        case MaterialsColorMap::none:
            switch (index)
            {
            case 0: // Default
            case 1: // Soma
                material.second->setDiffuseColor(Vector3f(0.9f, 0.9f, 0.9f));
                break;
            case 2: // Axon
                material.second->setDiffuseColor(Vector3f(0.2f, 0.2f, 0.8f));
                break;
            case 3: // Dendrite
                material.second->setDiffuseColor(Vector3f(0.8f, 0.2f, 0.2f));
                break;
            case 4: // Apical dendrite
                material.second->setDiffuseColor(Vector3f(0.8f, 0.2f, 0.8f));
                break;
            default:
                material.second->setDiffuseColor(
                    Vector3f(float(std::rand() % 255) / 255.f,
                             float(std::rand() % 255) / 255.f,
                             float(std::rand() % 255) / 255.f));
            }
            break;
        case MaterialsColorMap::gradient:
        {
            const float a = float(index) / float(_materials.size());
            material.second->setDiffuseColor(Vector3f(a, 0.f, 1.f - a));
            break;
        }
        case MaterialsColorMap::pastel:
            material.second->setDiffuseColor(
                Vector3f(0.5f + float(std::rand() % 127) / 255.f,
                         0.5f + float(std::rand() % 127) / 255.f,
                         0.5f + float(std::rand() % 127) / 255.f));
            break;
        case MaterialsColorMap::random:
            material.second->setDiffuseColor(
                Vector3f(float(rand() % 255) / 255.f,
                         float(rand() % 255) / 255.f,
                         float(rand() % 255) / 255.f));
            switch (rand() % 10)
            {
            case 0:
                // Transparency only
                material.second->setOpacity(float(std::rand() % 100) / 100.f);
                material.second->setRefractionIndex(1.2f);
                material.second->setSpecularColor(Vector3f(1.f));
                material.second->setSpecularExponent(10.f);
                break;
            case 1:
                // Light emission
                material.second->setEmission(std::rand() % 20);
                break;
            case 2:
                // Reflection only
                material.second->setReflectionIndex(float(std::rand() % 100) /
                                                    100.f);
                material.second->setSpecularColor(Vector3f(1.f));
                material.second->setSpecularExponent(10.f);
                break;
            case 3:
                // Reflection and refraction
                material.second->setReflectionIndex(float(std::rand() % 100) /
                                                    100.f);
                material.second->setOpacity(float(std::rand() % 100) / 100.f);
                material.second->setRefractionIndex(1.2f);
                material.second->setSpecularColor(Vector3f(1.f));
                material.second->setSpecularExponent(10.f);
                break;
            case 4:
                // Reflection and glossiness
                material.second->setReflectionIndex(float(std::rand() % 100) /
                                                    100.f);
                material.second->setSpecularColor(Vector3f(1.f));
                material.second->setSpecularExponent(10.f);
                material.second->setGlossiness(float(std::rand() % 100) /
                                               100.f);
                break;
            case 5:
                // Transparency and glossiness
                material.second->setOpacity(float(std::rand() % 100) / 100.f);
                material.second->setRefractionIndex(1.2f);
                material.second->setSpecularColor(Vector3f(1.f));
                material.second->setSpecularExponent(10.f);
                material.second->setGlossiness(float(std::rand() % 100) /
                                               100.f);
                break;
            }
            break;
        case MaterialsColorMap::shades_of_grey:
            float value = float(std::rand() % 255) / 255.f;
            material.second->setDiffuseColor(Vector3f(value, value, value));
            break;
        }
        material.second->commit();
        ++index;
    }
}

void Model::logInformation()
{
    _sizeInBytes = 0;
    uint64_t nbSpheres = 0;
    uint64_t nbCylinders = 0;
    uint64_t nbCones = 0;
    uint64_t nbMeshes = 0;
    for (const auto& spheres : _spheres)
    {
        _sizeInBytes += spheres.second.size() * sizeof(Sphere);
        nbSpheres += spheres.second.size();
    }
    for (const auto& cylinders : _cylinders)
    {
        _sizeInBytes += cylinders.second.size() * sizeof(Cylinder);
        nbCylinders += cylinders.second.size();
    }
    for (const auto& cones : _cones)
    {
        _sizeInBytes += cones.second.size() * sizeof(Cones);
        nbCones += cones.second.size();
    }
    for (const auto& trianglesMesh : _trianglesMeshes)
    {
        const auto& mesh = trianglesMesh.second;
        _sizeInBytes += mesh.indices.size() * sizeof(Vector3f);
        _sizeInBytes += mesh.normals.size() * sizeof(Vector3f);
        _sizeInBytes += mesh.colors.size() * sizeof(Vector4f);
        _sizeInBytes += mesh.indices.size() * sizeof(Vector3ui);
        _sizeInBytes += mesh.textureCoordinates.size() * sizeof(Vector2f);
        ++nbMeshes;
    }

    BRAYNS_INFO << "Spheres: " << nbSpheres << ", Cylinders: " << nbCylinders
                << ", Cones: " << nbCones << ", Meshes: " << nbMeshes
                << ", Memory: " << _sizeInBytes << " bytes ("
                << _sizeInBytes / 1048576 << " MB), Bounds: " << _bounds
                << std::endl;
}

MaterialPtr Model::getMaterial(const size_t materialId) const
{
    const auto it = _materials.find(materialId);
    if (it == _materials.end())
        throw std::runtime_error("Material " + std::to_string(materialId) +
                                 " is not registered in the model");
    return it->second;
}

void Model::createMissingMaterials()
{
    std::set<size_t> materialIds;
    for (auto& spheres : _spheres)
        materialIds.insert(spheres.first);
    for (auto& cylinders : _cylinders)
        materialIds.insert(cylinders.first);
    for (auto& cones : _cones)
        materialIds.insert(cones.first);
    for (auto& meshes : _trianglesMeshes)
        materialIds.insert(meshes.first);

    for (const auto materialId : materialIds)
    {
        const auto it = _materials.find(materialId);
        if (it == _materials.end())
            createMaterial(materialId, std::to_string(materialId));
    }
}
}
