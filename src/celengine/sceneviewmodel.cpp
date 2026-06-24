// sceneviewmodel.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "sceneviewmodel.h"

#include "body.h"
#include "deepskyobj.h"
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
        selectionSnapshot.selection = selection;
        selectionSnapshot.positionKm = selection.getPosition(snapshot.time).offsetFromKm(UniversalCoord::Zero());
        selectionSnapshot.visible = selection.isVisible();
        selectionSnapshot.clickable = isClickable(selection);
        snapshot.selections.push_back(selectionSnapshot);
    }

    return snapshot;
}
