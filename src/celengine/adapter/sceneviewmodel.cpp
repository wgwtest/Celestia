// sceneviewmodel.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "sceneviewmodel.h"

#include <array>
#include <string>

#include "body.h"
#include "deepskyobj.h"
#include "location.h"
#include "selection.h"
#include "simulation.h"
#include "star.h"
#include "univcoord.h"

namespace
{

bool
isClickable(const Selection& selection)
{
    switch (selection.getType())
    {
    case SelectionType::Star:
        return selection.star() != nullptr;
    case SelectionType::Body:
        return selection.body() != nullptr && selection.body()->isClickable();
    case SelectionType::DeepSky:
        return selection.deepsky() != nullptr && selection.deepsky()->isClickable();
    default:
        return false;
    }
}

std::string
selectionTypeName(SelectionType type)
{
    switch (type)
    {
    case SelectionType::Star:
        return "star";
    case SelectionType::Body:
        return "body";
    case SelectionType::DeepSky:
        return "deepsky";
    case SelectionType::Location:
        return "location";
    default:
        return "none";
    }
}

std::string
selectionId(const Selection& selection)
{
    switch (selection.getType())
    {
    case SelectionType::Star:
        return selection.star() != nullptr ? std::to_string(selection.star()->getIndex()) : std::string();
    case SelectionType::Body:
        return selection.body() != nullptr ? selection.body()->getName(false) : std::string();
    case SelectionType::DeepSky:
        return selection.deepsky() != nullptr ? std::to_string(selection.deepsky()->getIndex()) : std::string();
    case SelectionType::Location:
        return selection.location() != nullptr ? selection.location()->getName(false) : std::string();
    default:
        return {};
    }
}

std::array<double, 3>
toPositionKm(const Eigen::Vector3d& positionKm)
{
    return { positionKm.x(), positionKm.y(), positionKm.z() };
}

} // end unnamed namespace

SceneViewSnapshot
SceneViewModel::buildSelectionSnapshot(const Simulation& simulation)
{
    SceneViewSnapshot snapshot;
    snapshot.time = simulation.getTime();

    Selection selection = simulation.getSelection();
    if (!selection.empty())
    {
        SceneSelectionSnapshot selectionSnapshot;
        selectionSnapshot.type = selectionTypeName(selection.getType());
        selectionSnapshot.id = selectionId(selection);
        selectionSnapshot.positionKm = toPositionKm(selection.getPosition(snapshot.time).offsetFromKm(UniversalCoord::Zero()));
        selectionSnapshot.visible = selection.isVisible();
        selectionSnapshot.clickable = isClickable(selection);
        snapshot.selections.push_back(selectionSnapshot);
    }

    return snapshot;
}
