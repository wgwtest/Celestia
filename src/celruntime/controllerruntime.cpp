// controllerruntime.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "controllerruntime.h"

namespace celestia::runtime
{

ControllerRuntime::ControllerRuntime() = default;

bool
ControllerRuntime::isInitialized() const
{
    return true;
}

controller::ControllerService&
ControllerRuntime::service()
{
    return service_;
}

const controller::ControllerService&
ControllerRuntime::service() const
{
    return service_;
}

} // namespace celestia::runtime
