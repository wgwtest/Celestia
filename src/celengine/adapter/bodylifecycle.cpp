// bodylifecycle.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/adapter/bodylifecycle.h>

#include <utility>
#include <vector>

namespace
{

std::vector<BodyLifecycleEvents::BodyCallback>&
destroyedCallbacks()
{
    static auto* const callbacks = new std::vector<BodyLifecycleEvents::BodyCallback>(); //NOSONAR
    return *callbacks;
}

std::vector<BodyLifecycleEvents::BodyCallback>&
defaultPropertiesResetCallbacks()
{
    static auto* const callbacks = new std::vector<BodyLifecycleEvents::BodyCallback>(); //NOSONAR
    return *callbacks;
}

std::vector<BodyLifecycleEvents::RingCallback>&
ringRemovedCallbacks()
{
    static auto* const callbacks = new std::vector<BodyLifecycleEvents::RingCallback>(); //NOSONAR
    return *callbacks;
}

std::vector<BodyLifecycleEvents::ShapeOverrideQuery>&
shapeOverrideQueries()
{
    static auto* const callbacks = new std::vector<BodyLifecycleEvents::ShapeOverrideQuery>(); //NOSONAR
    return *callbacks;
}

} // end unnamed namespace

void
BodyLifecycleEvents::addDestroyedCallback(BodyCallback callback)
{
    destroyedCallbacks().push_back(std::move(callback));
}

void
BodyLifecycleEvents::addDefaultPropertiesResetCallback(BodyCallback callback)
{
    defaultPropertiesResetCallbacks().push_back(std::move(callback));
}

void
BodyLifecycleEvents::addRingRemovedCallback(RingCallback callback)
{
    ringRemovedCallbacks().push_back(std::move(callback));
}

void
BodyLifecycleEvents::addShapeOverrideQuery(ShapeOverrideQuery query)
{
    shapeOverrideQueries().push_back(std::move(query));
}

void
BodyLifecycleEvents::notifyDestroyed(const Body* body)
{
    for (const auto& callback : destroyedCallbacks())
        callback(body);
}

void
BodyLifecycleEvents::notifyDefaultPropertiesReset(const Body* body)
{
    for (const auto& callback : defaultPropertiesResetCallbacks())
        callback(body);
}

void
BodyLifecycleEvents::notifyRingRemoved(const RingSystem* rings)
{
    for (const auto& callback : ringRemovedCallbacks())
        callback(rings);
}

bool
BodyLifecycleEvents::hasShapeOverride(const Body* body)
{
    for (const auto& query : shapeOverrideQueries())
    {
        if (query(body))
            return true;
    }

    return false;
}
