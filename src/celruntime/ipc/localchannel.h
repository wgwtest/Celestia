// localchannel.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <queue>

#include "channel.h"

namespace celestia::runtime::ipc
{

class LocalChannel final : public Channel
{
public:
    void send(RuntimeMessage) override;
    std::optional<RuntimeMessage> receive() override;

private:
    std::queue<RuntimeMessage> m_messages;
};

} // namespace celestia::runtime::ipc
