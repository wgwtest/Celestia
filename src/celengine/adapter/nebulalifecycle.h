// nebulalifecycle.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <functional>

class Nebula;

class NebulaLifecycleEvents
{
public:
    using NebulaCallback = std::function<void(const Nebula*)>;

    static void addDestroyedCallback(NebulaCallback);
    static void notifyDestroyed(const Nebula*);
};
