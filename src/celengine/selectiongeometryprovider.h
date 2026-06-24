// selectiongeometryprovider.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Geometry>

#include "meshmanager.h"

class Body;

class SelectionGeometryProvider
{
public:
    virtual ~SelectionGeometryProvider() = default;

    virtual celestia::engine::GeometryHandle geometryFor(const Body*) const = 0;
    virtual Eigen::Quaternionf geometryOrientationFor(const Body*) const = 0;
    virtual float geometryScaleFor(const Body*) const = 0;
    virtual const Geometry* findGeometry(celestia::engine::GeometryHandle) const = 0;
};
