// viewruntime.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string_view>

#include "viewframe.h"

namespace celestia::runtime
{

class ViewRuntime
{
public:
    virtual ~ViewRuntime() = default;

    virtual std::string_view id() const = 0;
    virtual void present(const ViewFrame&) {}
    virtual std::string_view lastFrameSummary() const { return {}; }
};

} // namespace celestia::runtime
