// orbitsampler.h
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <vector>

#include <Eigen/Core>
#include <celephem/orbit.h>

struct OrbitSample
{
    Eigen::Vector3d position;
    double t{ 0.0 };
    Eigen::Vector3d velocity;
    double boundingRadius{ 0.0 };
};

class OrbitSampler : public celestia::ephem::OrbitSampleProc
{
public:
    std::vector<OrbitSample> samples;

    OrbitSampler() = default;

    void sample(double t, const Eigen::Vector3d& position, const Eigen::Vector3d& velocity)
    {
        OrbitSample samp;
        samp.t = t;
        samp.position = position;
        samp.velocity = velocity;
        samples.push_back(samp);
    }
};
