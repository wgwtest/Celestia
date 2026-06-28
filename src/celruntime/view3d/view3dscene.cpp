// view3dscene.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "view3dscene.h"

namespace celestia::runtime::view3d
{

View3DSceneState
buildView3DSceneState(const protocol::SceneFrame& frame,
                      const std::filesystem::path& contentRoot)
{
    View3DSceneState state;
    state.sequence = frame.sequence;
    state.simulationTime = frame.simulationTime;
    state.cameraPositionX = frame.camera.position[0];
    state.cameraPositionY = frame.camera.position[1];
    state.cameraPositionZ = frame.camera.position[2];
    state.cameraOrientationX = frame.camera.orientation[0];
    state.cameraOrientationY = frame.camera.orientation[1];
    state.cameraOrientationZ = frame.camera.orientation[2];
    state.cameraOrientationW = frame.camera.orientation[3];
    state.cameraFov = frame.camera.fov;
    state.bodyCount = static_cast<std::uint64_t>(frame.bodies.size());
    state.starCount = static_cast<std::uint64_t>(frame.stars.size());
    state.deepSkyObjectCount = static_cast<std::uint64_t>(frame.deepSkyObjects.size());
    state.orbitCount = static_cast<std::uint64_t>(frame.orbits.size());
    state.labelCount = static_cast<std::uint64_t>(frame.labels.size());
    state.observerReferenceBodyId = frame.observer.referenceBodyId;
    state.observerFrame = frame.observer.frame;
    state.selectionType = frame.selection.type;
    state.selectionId = frame.selection.id;
    state.resourceCount = static_cast<std::uint64_t>(frame.resources.size());
    state.resources = resolveSceneResources(frame, contentRoot);

    for (const auto& resource : state.resources)
    {
        if (resource.status == View3DResourceStatus::Resolved)
            ++state.resolvedResourceCount;
        else if (resource.status == View3DResourceStatus::MissingRequired)
            ++state.missingRequiredResourceCount;
        else if (resource.status == View3DResourceStatus::Invalid)
            ++state.invalidResourceCount;
    }

    return state;
}

} // namespace celestia::runtime::view3d
