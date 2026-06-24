// sceneviewmodel.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <vector>

#include <Eigen/Core>

#include "selection.h"

struct SceneSelectionSnapshot
{
    Selection selection;
    Eigen::Vector3d positionKm{ Eigen::Vector3d::Zero() };
    bool visible{ false };
    bool clickable{ false };
};

struct SceneViewSnapshot
{
    double time{ 0.0 };
    std::vector<SceneSelectionSnapshot> selections;
};

class Simulation;

class SceneViewModel
{
public:
    static SceneViewSnapshot buildSelectionSnapshot(const Simulation&);
};
