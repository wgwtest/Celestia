// controllerhostmain.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "runtimehostcommon.h"

#include <iostream>

int
main(int argc, char* argv[])
{
    return celestia::runtime::process::runRuntimeHost("controller", argc, argv, std::cin, std::cout, std::cerr);
}
