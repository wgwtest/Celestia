// nebularenderassetloader.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/adapter/nebularenderassetloader.h>

#include <celutil/associativearray.h>
#include <celutil/fsutils.h>
#include <celutil/logger.h>
#include <celengine/view3d/meshmanager.h>
#include <celengine/model/nebula.h>
#include <celengine/adapter/nebularenderassets.h>

namespace engine = celestia::engine;
namespace util = celestia::util;

bool
NebulaRenderAssetLoader::load(Nebula& nebula,
                              const util::AssociativeArray* params,
                              const std::filesystem::path& resPath,
                              engine::GeometryPaths& geometryPaths)
{
    if (const std::string* meshName = params->getString("Mesh"); meshName != nullptr)
    {
        auto geometryFileName = util::U8FileName(*meshName);
        if (!geometryFileName.has_value())
        {
            util::GetLogger()->error("Invalid filename in Mesh\n");
            return false;
        }

        auto geometryHandle = geometryPaths.getHandle(*geometryFileName, resPath);
        NebulaRenderAssets::setGeometry(&nebula, geometryHandle);
    }

    return true;
}
