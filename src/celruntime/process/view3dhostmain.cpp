// view3dhostmain.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "runtimehostcommon.h"

#include <iostream>
#include <string_view>

#ifdef CELESTIA_RUNTIME_VIEW3D_SDL
#include "view3dhostloop.h"

#include <celruntime/view3d/view3dloop.h>
#endif

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace
{

bool
useBinaryStdio(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::string_view(argv[i] != nullptr ? argv[i] : "") == "--serve")
            return true;
    }

    return false;
}

} // end unnamed namespace

#ifdef CELESTIA_RUNTIME_VIEW3D_SDL
namespace
{

celestia::runtime::view3d::View3DLoop&
view3DLoop()
{
    static celestia::runtime::view3d::View3DLoop loop;
    return loop;
}

bool
startView3DLoop(std::string& errorMessage)
{
    celestia::runtime::view3d::View3DLoopOptions options;
    options.title = "Celestia MVC View3D Host";
    options.width = 960;
    options.height = 540;
    options.visible = true;
    return view3DLoop().start(options, &errorMessage);
}

bool
renderView3DFrame(const celestia::runtime::protocol::SceneFrame& frame, std::string& errorMessage)
{
    return view3DLoop().render(frame, &errorMessage);
}

std::vector<celestia::runtime::protocol::ViewInputEvent>
drainView3DInput()
{
    return view3DLoop().drainInputEvents();
}

void
stopView3DLoop()
{
    view3DLoop().stop();
}

} // end unnamed namespace
#endif

int
main(int argc, char* argv[])
{
#ifdef _WIN32
    if (useBinaryStdio(argc, argv))
    {
        _setmode(_fileno(stdin), _O_BINARY);
        _setmode(_fileno(stdout), _O_BINARY);
    }
#endif
#ifdef CELESTIA_RUNTIME_VIEW3D_SDL
    celestia::runtime::process::setRuntimeView3DHostCallbacks({
        startView3DLoop,
        renderView3DFrame,
        drainView3DInput,
        stopView3DLoop,
    });
#endif
    return celestia::runtime::process::runRuntimeHost("view3d", argc, argv, std::cin, std::cout, std::cerr);
}
