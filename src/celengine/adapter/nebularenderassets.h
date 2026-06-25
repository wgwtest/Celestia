// nebularenderassets.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <celengine/view3d/meshmanager.h>

class Nebula;

class NebulaRenderAssets
{
public:
    static celestia::engine::GeometryHandle getGeometry(const Nebula*);
    static void setGeometry(const Nebula*, celestia::engine::GeometryHandle);
    static void remove(const Nebula*);
};
