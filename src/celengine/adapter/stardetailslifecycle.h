// stardetailslifecycle.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <functional>

#include "stellarclass.h"

class StarDetails;

class StarDetailsLifecycleEvents
{
public:
    using DestroyedCallback = std::function<void(const StarDetails*)>;
    using CloneCallback = std::function<void(const StarDetails*, const StarDetails*)>;
    using MergeCallback = std::function<void(StarDetails*, const StarDetails*)>;
    using SpectralDefaultCallback = std::function<void(const StarDetails*, StellarClass::SpectralClass)>;
    using PlainDefaultCallback = std::function<void(const StarDetails*)>;

    static void addDestroyedCallback(DestroyedCallback);
    static void addCloneCallback(CloneCallback);
    static void addCopyCallback(CloneCallback);
    static void addCopyTextureIfUnsetCallback(MergeCallback);
    static void addNormalStarDefaultCallback(SpectralDefaultCallback);
    static void addWhiteDwarfDefaultCallback(PlainDefaultCallback);
    static void addNeutronStarDefaultCallback(PlainDefaultCallback);

    static void notifyDestroyed(const StarDetails*);
    static void notifyCloned(const StarDetails*, const StarDetails*);
    static void notifyCopied(const StarDetails*, const StarDetails*);
    static void notifyCopyTextureIfUnset(StarDetails*, const StarDetails*);
    static void notifyNormalStarDefault(const StarDetails*, StellarClass::SpectralClass);
    static void notifyWhiteDwarfDefault(const StarDetails*);
    static void notifyNeutronStarDefault(const StarDetails*);
};
