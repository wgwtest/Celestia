// view3dhostloop.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <iosfwd>
#include <string>
#include <vector>

#include <celruntime/protocol/sceneprotocol.h>
#include <celruntime/protocol/viewinput.h>

namespace celestia::runtime::process
{

struct View3DHostCallbacks
{
    bool (*start)(std::string& errorMessage){ nullptr };
    bool (*render)(const protocol::SceneFrame& frame, std::string& errorMessage){ nullptr };
    std::vector<protocol::ViewInputEvent> (*drainInput)(){ nullptr };
    void (*stop)(){ nullptr };
};

void setRuntimeView3DHostCallbacks(View3DHostCallbacks callbacks);

int runRuntimeView3DHostLoop(std::string sessionId,
                             std::istream& input,
                             std::ostream& output,
                             std::ostream& error);

} // namespace celestia::runtime::process
