// deepskyobjectpicker.h
//
// Copyright (C) 2003-2009, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Geometry>

class DeepSkyObject;

bool pickDeepSkyObjectGeometry(const DeepSkyObject&,
                               const Eigen::ParametrizedLine<double, 3>& ray,
                               double& distanceToPicker,
                               double& cosAngleToBoundCenter);
