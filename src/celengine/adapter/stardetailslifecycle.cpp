// stardetailslifecycle.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/adapter/stardetailslifecycle.h>

#include <utility>
#include <vector>

namespace
{

std::vector<StarDetailsLifecycleEvents::DestroyedCallback>&
destroyedCallbacks()
{
    static auto* const callbacks = new std::vector<StarDetailsLifecycleEvents::DestroyedCallback>(); //NOSONAR
    return *callbacks;
}

std::vector<StarDetailsLifecycleEvents::CloneCallback>&
cloneCallbacks()
{
    static auto* const callbacks = new std::vector<StarDetailsLifecycleEvents::CloneCallback>(); //NOSONAR
    return *callbacks;
}

std::vector<StarDetailsLifecycleEvents::CloneCallback>&
copyCallbacks()
{
    static auto* const callbacks = new std::vector<StarDetailsLifecycleEvents::CloneCallback>(); //NOSONAR
    return *callbacks;
}

std::vector<StarDetailsLifecycleEvents::MergeCallback>&
copyTextureIfUnsetCallbacks()
{
    static auto* const callbacks = new std::vector<StarDetailsLifecycleEvents::MergeCallback>(); //NOSONAR
    return *callbacks;
}

std::vector<StarDetailsLifecycleEvents::SpectralDefaultCallback>&
normalStarDefaultCallbacks()
{
    static auto* const callbacks = new std::vector<StarDetailsLifecycleEvents::SpectralDefaultCallback>(); //NOSONAR
    return *callbacks;
}

std::vector<StarDetailsLifecycleEvents::PlainDefaultCallback>&
whiteDwarfDefaultCallbacks()
{
    static auto* const callbacks = new std::vector<StarDetailsLifecycleEvents::PlainDefaultCallback>(); //NOSONAR
    return *callbacks;
}

std::vector<StarDetailsLifecycleEvents::PlainDefaultCallback>&
neutronStarDefaultCallbacks()
{
    static auto* const callbacks = new std::vector<StarDetailsLifecycleEvents::PlainDefaultCallback>(); //NOSONAR
    return *callbacks;
}

} // end unnamed namespace

void
StarDetailsLifecycleEvents::addDestroyedCallback(DestroyedCallback callback)
{
    destroyedCallbacks().push_back(std::move(callback));
}

void
StarDetailsLifecycleEvents::addCloneCallback(CloneCallback callback)
{
    cloneCallbacks().push_back(std::move(callback));
}

void
StarDetailsLifecycleEvents::addCopyCallback(CloneCallback callback)
{
    copyCallbacks().push_back(std::move(callback));
}

void
StarDetailsLifecycleEvents::addCopyTextureIfUnsetCallback(MergeCallback callback)
{
    copyTextureIfUnsetCallbacks().push_back(std::move(callback));
}

void
StarDetailsLifecycleEvents::addNormalStarDefaultCallback(SpectralDefaultCallback callback)
{
    normalStarDefaultCallbacks().push_back(std::move(callback));
}

void
StarDetailsLifecycleEvents::addWhiteDwarfDefaultCallback(PlainDefaultCallback callback)
{
    whiteDwarfDefaultCallbacks().push_back(std::move(callback));
}

void
StarDetailsLifecycleEvents::addNeutronStarDefaultCallback(PlainDefaultCallback callback)
{
    neutronStarDefaultCallbacks().push_back(std::move(callback));
}

void
StarDetailsLifecycleEvents::notifyDestroyed(const StarDetails* details)
{
    for (const auto& callback : destroyedCallbacks())
        callback(details);
}

void
StarDetailsLifecycleEvents::notifyCloned(const StarDetails* source, const StarDetails* target)
{
    for (const auto& callback : cloneCallbacks())
        callback(source, target);
}

void
StarDetailsLifecycleEvents::notifyCopied(const StarDetails* target, const StarDetails* source)
{
    for (const auto& callback : copyCallbacks())
        callback(target, source);
}

void
StarDetailsLifecycleEvents::notifyCopyTextureIfUnset(StarDetails* target, const StarDetails* source)
{
    for (const auto& callback : copyTextureIfUnsetCallbacks())
        callback(target, source);
}

void
StarDetailsLifecycleEvents::notifyNormalStarDefault(const StarDetails* details, StellarClass::SpectralClass spectralClass)
{
    for (const auto& callback : normalStarDefaultCallbacks())
        callback(details, spectralClass);
}

void
StarDetailsLifecycleEvents::notifyWhiteDwarfDefault(const StarDetails* details)
{
    for (const auto& callback : whiteDwarfDefaultCallbacks())
        callback(details);
}

void
StarDetailsLifecycleEvents::notifyNeutronStarDefault(const StarDetails* details)
{
    for (const auto& callback : neutronStarDefaultCallbacks())
        callback(details);
}
