// framedmessage.h
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

#include <celruntime/protocol/envelope.h>

namespace celestia::runtime::transport
{

enum class ReceiveStatus
{
    Message,
    Closed,
    Malformed,
    WouldBlock,
};

struct ReceiveResult
{
    ReceiveStatus status{ ReceiveStatus::WouldBlock };
    std::optional<protocol::RuntimeEnvelope> message;
};

std::string encodeFrame(const protocol::RuntimeEnvelope&);

class FramedMessageReader
{
public:
    void append(std::string_view);
    ReceiveResult receive();
    void close();

private:
    std::string m_buffer;
    bool m_closed{ false };
    bool m_malformed{ false };
};

} // namespace celestia::runtime::transport
