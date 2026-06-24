// opencluster.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>

#include <Eigen/Geometry>

#include "deepskyobj.h"

class OpenCluster final : public DeepSkyObject
{
public:
    OpenCluster() = default;

    const char* getType() const override;
    void setType(const std::string&) override;
    std::string getDescription() const override;

    DeepSkyObjectType getObjType() const override;

    enum ClusterType
    {
        Open          = 0,
        Globular      = 1,
        NotDefined    = 2
    };

protected:
    bool loadDetails(const celestia::util::AssociativeArray*,
                     const std::filesystem::path&) override
    {
        // nothing to load
        return true;
    }
};
