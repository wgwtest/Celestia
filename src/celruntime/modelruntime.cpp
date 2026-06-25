// modelruntime.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "modelruntime.h"

namespace celestia::runtime
{

ModelRuntime::ModelRuntime() = default;

bool
ModelRuntime::isInitialized() const
{
    return true;
}

model::ModelService&
ModelRuntime::service()
{
    return service_;
}

const model::ModelService&
ModelRuntime::service() const
{
    return service_;
}

} // namespace celestia::runtime
