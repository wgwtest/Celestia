// selectionpicker.h
//
// Copyright (C) 2001-2009, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Core>

#include <celengine/renderflags.h>
#include <celengine/selection.h>
#include <celengine/univcoord.h>

class SolarSystem;
class Universe;

namespace celestia::engine
{
class GeometryManager;
}

class SelectionPicker
{
public:
    SelectionPicker(const Universe&, celestia::engine::GeometryManager&);

    Selection pick(const UniversalCoord& origin,
                   const Eigen::Vector3f& direction,
                   double when,
                   RenderFlags renderFlags,
                   float faintestMag,
                   float tolerance = 0.0f) const;

private:
    Selection pickPlanet(const SolarSystem& solarSystem,
                         const UniversalCoord& origin,
                         const Eigen::Vector3f& direction,
                         double when,
                         float faintestMag,
                         float tolerance) const;

    Selection pickStar(const UniversalCoord& origin,
                       const Eigen::Vector3f& direction,
                       double when,
                       float faintest,
                       float tolerance = 0.0f) const;

    Selection pickDeepSkyObject(const UniversalCoord& origin,
                                const Eigen::Vector3f& direction,
                                RenderFlags renderFlags,
                                float faintest,
                                float tolerance = 0.0f) const;

    const Universe& universe;
    celestia::engine::GeometryManager& geometryManager;
};
