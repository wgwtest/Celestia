// localchannel.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "localchannel.h"

#include <utility>

namespace celestia::runtime::ipc
{

void
LocalChannel::send(RuntimeMessage message)
{
    m_messages.push(std::move(message));
}

std::optional<RuntimeMessage>
LocalChannel::receive()
{
    if (m_messages.empty())
        return std::nullopt;

    auto message = std::move(m_messages.front());
    m_messages.pop();
    return message;
}

} // namespace celestia::runtime::ipc
