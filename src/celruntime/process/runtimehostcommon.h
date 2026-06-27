// runtimehostcommon.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <iosfwd>
#include <memory>
#include <string_view>

#include <celruntime/model/modelsnapshot.h>

namespace celestia::runtime::process
{

int runRuntimeHost(std::string_view role,
                   int argc,
                   char* argv[],
                   std::istream& input,
                   std::ostream& output,
                   std::ostream& error,
                   std::unique_ptr<model::SimulationBackend> modelBackend = nullptr);

} // namespace celestia::runtime::process
