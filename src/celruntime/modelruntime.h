// modelruntime.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include "model/modelservice.h"

namespace celestia::runtime
{

class ModelRuntime
{
public:
    ModelRuntime();

    bool isInitialized() const;

    model::ModelService& service();
    const model::ModelService& service() const;

private:
    model::ModelService service_;
};

} // namespace celestia::runtime
