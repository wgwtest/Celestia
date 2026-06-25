// envelope.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace celestia::runtime::protocol
{

inline constexpr int CurrentProtocolVersion{ 1 };

enum class RuntimeRole
{
    Launcher,
    Model,
    Controller,
    View,
    Broadcast,
};

enum class RuntimeMessageKind
{
    Lifecycle,
    Command,
    Event,
    ViewFrame,
    Heartbeat,
    Error,
};

std::string_view roleName(RuntimeRole);
std::optional<RuntimeRole> roleFromString(std::string_view);

std::string_view messageKindName(RuntimeMessageKind);
std::optional<RuntimeMessageKind> messageKindFromString(std::string_view);

struct RuntimeEnvelope
{
    int protocolVersion{ CurrentProtocolVersion };
    std::string sessionId;
    std::uint64_t sequenceId{ 0 };
    std::int64_t timestampUs{ 0 };
    RuntimeRole sourceRole{ RuntimeRole::Launcher };
    RuntimeRole targetRole{ RuntimeRole::Broadcast };
    RuntimeMessageKind kind{ RuntimeMessageKind::Lifecycle };
    std::string name;
    std::string payload;
};

RuntimeEnvelope lifecycle(RuntimeRole source, RuntimeRole target, std::string name);
RuntimeEnvelope hello(RuntimeRole source, RuntimeRole target);
RuntimeEnvelope ready(RuntimeRole source, RuntimeRole target);
RuntimeEnvelope shutdown(RuntimeRole source, RuntimeRole target);

std::string serializeEnvelope(const RuntimeEnvelope&);
std::optional<RuntimeEnvelope> deserializeEnvelope(std::string_view);

} // namespace celestia::runtime::protocol
