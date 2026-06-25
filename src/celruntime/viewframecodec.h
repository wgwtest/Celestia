// viewframecodec.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "viewframe.h"

namespace celestia::runtime
{

std::string serializeViewFrame(const ViewFrame&);
std::optional<ViewFrame> deserializeViewFrame(std::string_view);

} // namespace celestia::runtime
