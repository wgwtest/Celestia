// nebularenderassets.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "nebularenderassets.h"

#include <unordered_map>

#include "nebula.h"
#include "nebulalifecycle.h"

namespace engine = celestia::engine;

namespace
{

std::unordered_map<const Nebula*, engine::GeometryHandle>&
geometryHandles()
{
    static auto* const handles = new std::unordered_map<const Nebula*, engine::GeometryHandle>(); //NOSONAR
    return *handles;
}

void
ensureNebulaLifecycleRegistration()
{
    static const bool registered = []()
    {
        NebulaLifecycleEvents::addDestroyedCallback([](const Nebula* nebula)
        {
            NebulaRenderAssets::remove(nebula);
        });
        return true;
    }();
    (void)registered;
}

} // end unnamed namespace

engine::GeometryHandle
NebulaRenderAssets::getGeometry(const Nebula* nebula)
{
    ensureNebulaLifecycleRegistration();
    auto& handles = geometryHandles();
    auto it = handles.find(nebula);
    return it == handles.end() ? engine::GeometryHandle::Invalid : it->second;
}

void
NebulaRenderAssets::setGeometry(const Nebula* nebula, engine::GeometryHandle geometry)
{
    ensureNebulaLifecycleRegistration();
    geometryHandles()[nebula] = geometry;
}

void
NebulaRenderAssets::remove(const Nebula* nebula)
{
    ensureNebulaLifecycleRegistration();
    geometryHandles().erase(nebula);
}
