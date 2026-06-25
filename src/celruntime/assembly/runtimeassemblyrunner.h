// runtimeassemblyrunner.h
//
// Copyright (C) 2026, the Celestia Development Team

#pragma once

#include "runtimeassemblyconfig.h"

#include <celruntime/process/runtimesession.h>

namespace celestia::runtime::assembly
{

class RuntimeAssemblyRunner
{
public:
    explicit RuntimeAssemblyRunner(RuntimeAssemblyConfig);

    process::RuntimeSessionResult run() const;

private:
    RuntimeAssemblyConfig config_;
};

} // namespace celestia::runtime::assembly
