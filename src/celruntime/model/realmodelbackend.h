// realmodelbackend.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>

#include "modelsnapshot.h"

namespace celestia::runtime::model
{

std::unique_ptr<SimulationBackend> createRealModelBackend();

} // namespace celestia::runtime::model
