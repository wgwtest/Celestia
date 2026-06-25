// runtimehostloop.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>

#include <celruntime/protocol/envelope.h>
#include <celruntime/transport/framedtransport.h>

namespace celestia::runtime::process
{

int runRuntimeHostLoop(celestia::runtime::protocol::RuntimeRole role,
                       std::string sessionId,
                       std::istream& input,
                       std::ostream& output,
                       std::ostream& error);

int runRuntimeHostLoop(celestia::runtime::protocol::RuntimeRole role,
                       std::string sessionId,
                       celestia::runtime::transport::FramedTransport& transport,
                       std::ostream& error);

std::optional<celestia::runtime::protocol::RuntimeRole> runtimeRoleFromHostRole(std::string_view);

} // namespace celestia::runtime::process
