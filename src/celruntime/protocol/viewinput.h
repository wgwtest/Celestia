// viewinput.h
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
#include <optional>
#include <string>
#include <string_view>

#include <celruntime/protocol/envelope.h>

namespace celestia::runtime::protocol
{

inline constexpr const char* ViewInputMessageName{ "view.input" };

struct ViewInputEvent
{
    std::string sessionId;
    std::uint64_t sequence{ 0 };
    std::string device;
    std::string action;
    std::string key;
    std::array<double, 2> pointer{ 0.0, 0.0 };
    std::array<double, 2> wheel{ 0.0, 0.0 };
    std::string modifiers;
};

std::string serializeViewInputEvent(const ViewInputEvent&);
std::optional<ViewInputEvent> deserializeViewInputEvent(std::string_view);

RuntimeEnvelope viewInputEnvelope(const ViewInputEvent&, RuntimeRole source, RuntimeRole target);

} // namespace celestia::runtime::protocol
