// message.h
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

#include <celruntime/viewframe.h>

namespace celestia::runtime::ipc
{

inline constexpr int CurrentProtocolVersion{ 1 };

enum class MessageKind
{
    Command,
    Event,
    ViewFrame,
    Error,
};

std::string_view messageKindName(MessageKind);
std::optional<MessageKind> messageKindFromString(std::string_view);

struct RuntimeMessage
{
    int protocolVersion{ CurrentProtocolVersion };
    MessageKind kind{ MessageKind::Command };
    std::string name;
    std::string payload;
    celestia::runtime::ViewFrame frame;

    static RuntimeMessage command(std::string, std::string);
    static RuntimeMessage event(std::string, std::string);
    static RuntimeMessage viewFrame(std::string, celestia::runtime::ViewFrame);
    static RuntimeMessage error(std::string, std::string);
};

std::string serializeMessage(const RuntimeMessage&);
std::optional<RuntimeMessage> deserializeMessage(std::string_view);

} // namespace celestia::runtime::ipc
