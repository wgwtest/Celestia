// controllerruntime.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include "controller/controllerservice.h"

namespace celestia::runtime
{

class ControllerRuntime
{
public:
    ControllerRuntime();

    bool isInitialized() const;

    controller::ControllerService& service();
    const controller::ControllerService& service() const;

private:
    controller::ControllerService service_;
};

} // namespace celestia::runtime
