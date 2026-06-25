// view3dloop.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <celruntime/protocol/sceneprotocol.h>
#include <celruntime/protocol/viewinput.h>

namespace celestia::runtime::view3d
{

struct View3DLoopOptions
{
    int width{ 960 };
    int height{ 540 };
    bool visible{ true };
    std::string title{ "Celestia MVC View3D Host" };
};

class View3DLoop
{
public:
    View3DLoop();
    ~View3DLoop();

    View3DLoop(const View3DLoop&) = delete;
    View3DLoop& operator=(const View3DLoop&) = delete;

    bool start(const View3DLoopOptions& options, std::string* errorMessage = nullptr);
    bool render(const protocol::SceneFrame& frame, std::string* errorMessage = nullptr);
    std::vector<protocol::ViewInputEvent> drainInputEvents();
    void stop();

    bool isRunning() const;
    std::uint64_t renderedFrameCount() const;
    std::string rendererInfo() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace celestia::runtime::view3d
