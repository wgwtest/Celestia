// viewprovider.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <functional>
#include <memory>
#include <string>

#include "viewruntime.h"

namespace celestia::runtime
{

struct ViewProvider
{
    std::string id;
    std::string displayName;
    std::string dimension;
    std::function<std::unique_ptr<ViewRuntime>()> create;
};

} // namespace celestia::runtime
