# Copyright (c) 2015-2017, EPFL/Blue Brain Project
# Responsible Author: Cyrille Favreau <cyrille.favreau@epfl.ch>
#
# This file is part of Brayns <https://github.com/BlueBrain/Brayns>

set(BRAYNSCOMMON_SOURCES
  ImageManager.cpp
  PropertyMap.cpp
  input/KeyboardHandler.cpp
  light/DirectionalLight.cpp
  light/Light.cpp
  light/PointLight.cpp
  loader/LoaderRegistry.cpp
  material/Texture2D.cpp
  scene/ClipPlane.cpp
  simulation/AbstractSimulationHandler.cpp
  transferFunction/TransferFunction.cpp
  utils/base64/base64.cpp
  utils/DynamicLib.cpp
  utils/imageUtils.cpp
  utils/utils.cpp
)

set(BRAYNSCOMMON_PUBLIC_HEADERS
  ActionInterface.h
  BaseObject.h
  ImageManager.h
  Progress.h
  PropertyMap.h
  PropertyObject.h
  Statistics.h
  Timer.h
  Transformation.h
  geometry/CommonDefines.h
  geometry/Cone.h
  geometry/Cylinder.h
  geometry/SDFGeometry.h
  geometry/Sphere.h
  geometry/Streamline.h
  geometry/TrianglesMesh.h
  input/KeyboardHandler.h
  light/DirectionalLight.h
  light/Light.h
  light/PointLight.h
  loader/Loader.h
  loader/LoaderRegistry.h
  log.h
  material/Texture2D.h
  mathTypes.h
  scene/ClipPlane.h
  simulation/AbstractSimulationHandler.h
  tasks/Task.h
  tasks/TaskFunctor.h
  tasks/TaskRuntimeError.h
  transferFunction/TransferFunction.h
  types.h
  utils/enumUtils.h
  utils/utils.h
  utils/imageUtils.h
)

set(BRAYNSCOMMON_HEADERS
  utils/DynamicLib.h
  utils/base64/base64.h
)

set(BRAYNSCOMMON_INCLUDE_NAME brayns/common)
set(BRAYNSCOMMON_LINK_LIBRARIES
  PUBLIC vmmlib Async++
  PRIVATE braynsParameters ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_SYSTEM_LIBRARY}
    ${FREEIMAGE_LIBRARIES}
)

common_library(braynsCommon)
target_include_directories(braynsCommon SYSTEM PRIVATE ${FREEIMAGE_INCLUDE_DIRS})
