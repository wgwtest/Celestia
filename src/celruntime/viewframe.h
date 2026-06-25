// viewframe.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace celestia::runtime
{

struct ViewFrameSelection
{
    std::string type;
    std::string id;
    std::array<double, 3> positionKm{ 0.0, 0.0, 0.0 };
    bool visible{ false };
    bool clickable{ false };
};

struct ViewFrame
{
    std::uint64_t frameId{ 0 };
    double time{ 0.0 };
    std::vector<ViewFrameSelection> selections;
    std::string summary;
};

} // namespace celestia::runtime
