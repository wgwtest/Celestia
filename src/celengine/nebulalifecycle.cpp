// nebulalifecycle.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "nebulalifecycle.h"

#include <utility>
#include <vector>

namespace
{

std::vector<NebulaLifecycleEvents::NebulaCallback>&
destroyedCallbacks()
{
    static auto* const callbacks = new std::vector<NebulaLifecycleEvents::NebulaCallback>(); //NOSONAR
    return *callbacks;
}

} // end unnamed namespace

void
NebulaLifecycleEvents::addDestroyedCallback(NebulaCallback callback)
{
    destroyedCallbacks().push_back(std::move(callback));
}

void
NebulaLifecycleEvents::notifyDestroyed(const Nebula* nebula)
{
    for (const auto& callback : destroyedCallbacks())
        callback(nebula);
}
