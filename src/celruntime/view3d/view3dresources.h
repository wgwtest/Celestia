// view3dresources.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <filesystem>
#include <vector>

#include <celruntime/protocol/sceneprotocol.h>

namespace celestia::runtime::view3d
{

struct View3DResolvedResource
{
    protocol::ResourceRef resource;
    std::filesystem::path resolvedPath;
    bool exists{ false };
};

std::vector<View3DResolvedResource>
resolveSceneResources(const protocol::SceneFrame& frame,
                      const std::filesystem::path& contentRoot);

} // namespace celestia::runtime::view3d
