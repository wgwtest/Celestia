// modelhostmain.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "runtimehostcommon.h"

#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include <celruntime/model/realmodelbackend.h>

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

bool
hasDataRoot(int argc, char* argv[])
{
    constexpr std::string_view dataRootOption{ "--data-root=" };
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view argument{ argv[i] != nullptr ? argv[i] : "" };
        if (argument.substr(0, dataRootOption.size()) == dataRootOption &&
            argument.size() > dataRootOption.size())
        {
            return true;
        }
    }

    return false;
}

} // end unnamed namespace

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
    auto backend = hasDataRoot(argc, argv)
        ? celestia::runtime::model::createRealModelBackend()
        : std::unique_ptr<celestia::runtime::model::SimulationBackend>{};
    return celestia::runtime::process::runRuntimeHost("model", argc, argv, std::cin, std::cout, std::cerr,
                                                      std::move(backend));
}
