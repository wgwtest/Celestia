// lifecycle.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

namespace celestia::runtime::protocol
{

inline constexpr const char* RuntimeHello{ "runtime.hello" };
inline constexpr const char* RuntimeReady{ "runtime.ready" };
inline constexpr const char* RuntimeStart{ "runtime.start" };
inline constexpr const char* RuntimePause{ "runtime.pause" };
inline constexpr const char* RuntimeResume{ "runtime.resume" };
inline constexpr const char* RuntimeShutdown{ "runtime.shutdown" };
inline constexpr const char* RuntimeStopped{ "runtime.stopped" };
inline constexpr const char* RuntimeHeartbeat{ "runtime.heartbeat" };
inline constexpr const char* RuntimeError{ "runtime.error" };

} // namespace celestia::runtime::protocol
