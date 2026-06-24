// deepskyobjectrenderpolicy.cpp
//
// Copyright (C) 2003-2009, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "deepskyobjectrenderpolicy.h"

#include "deepskyobj.h"

RenderFlags
getDeepSkyObjectRenderMask(const DeepSkyObject& dso)
{
    switch (dso.getObjType())
    {
    case DeepSkyObjectType::Galaxy:
        return RenderFlags::ShowGalaxies;
    case DeepSkyObjectType::Globular:
        return RenderFlags::ShowGlobulars;
    case DeepSkyObjectType::Nebula:
        return RenderFlags::ShowNebulae;
    case DeepSkyObjectType::OpenCluster:
        return RenderFlags::ShowOpenClusters;
    default:
        return RenderFlags::ShowNothing;
    }
}

RenderLabels
getDeepSkyObjectLabelMask(const DeepSkyObject& dso)
{
    switch (dso.getObjType())
    {
    case DeepSkyObjectType::Galaxy:
        return RenderLabels::GalaxyLabels;
    case DeepSkyObjectType::Globular:
        return RenderLabels::GlobularLabels;
    case DeepSkyObjectType::Nebula:
        return RenderLabels::NebulaLabels;
    case DeepSkyObjectType::OpenCluster:
        return RenderLabels::OpenClusterLabels;
    default:
        return RenderLabels::NoLabels;
    }
}
