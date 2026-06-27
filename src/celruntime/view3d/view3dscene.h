// view3dscene.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <celruntime/protocol/sceneprotocol.h>
#include <celruntime/view3d/view3dresources.h>

namespace celestia::runtime::view3d
{

struct View3DSceneState
{
    std::uint64_t sequence{ 0 };
    double simulationTime{ 0.0 };
    double cameraFov{ 0.0 };
    std::uint64_t bodyCount{ 0 };
    std::uint64_t starCount{ 0 };
    std::uint64_t deepSkyObjectCount{ 0 };
    std::uint64_t orbitCount{ 0 };
    std::uint64_t labelCount{ 0 };
    std::string selectionType;
    std::string selectionId;
    std::uint64_t resourceCount{ 0 };
    std::uint64_t resolvedResourceCount{ 0 };
    std::uint64_t missingRequiredResourceCount{ 0 };
    std::vector<View3DResolvedResource> resources;
};

View3DSceneState
buildView3DSceneState(const protocol::SceneFrame& frame,
                      const std::filesystem::path& contentRoot);

} // namespace celestia::runtime::view3d
