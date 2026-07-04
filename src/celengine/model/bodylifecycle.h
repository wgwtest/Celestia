// bodylifecycle.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <functional>

class Body;
struct RingSystem;

class BodyLifecycleEvents
{
public:
    using BodyCallback = std::function<void(const Body*)>;
    using RingCallback = std::function<void(const RingSystem*)>;
    using ShapeOverrideQuery = std::function<bool(const Body*)>;

    static void addDestroyedCallback(BodyCallback);
    static void addDefaultPropertiesResetCallback(BodyCallback);
    static void addRingRemovedCallback(RingCallback);
    static void addShapeOverrideQuery(ShapeOverrideQuery);

    static void notifyDestroyed(const Body*);
    static void notifyDefaultPropertiesReset(const Body*);
    static void notifyRingRemoved(const RingSystem*);

    static bool hasShapeOverride(const Body*);
};
