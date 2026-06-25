// hostprocess.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "hostprocess.h"

#ifndef _WIN32

#include <utility>

namespace celestia::runtime::process
{

class HostProcess::Impl
{
public:
    explicit Impl(HostProcessOptions options)
        : options_(std::move(options))
    {
    }

    bool start()
    {
        return false;
    }

    bool send(const protocol::RuntimeEnvelope&)
    {
        return false;
    }

    std::optional<protocol::RuntimeEnvelope> receive(std::chrono::milliseconds)
    {
        return std::nullopt;
    }

    std::optional<int> wait(std::chrono::milliseconds)
    {
        return std::nullopt;
    }

    bool terminate()
    {
        return false;
    }

    bool running() const
    {
        return false;
    }

private:
    HostProcessOptions options_;
};

HostProcess::HostProcess(HostProcessOptions options)
    : impl_(std::make_unique<Impl>(std::move(options)))
{
}

HostProcess::~HostProcess() = default;

bool
HostProcess::start()
{
    return impl_->start();
}

bool
HostProcess::send(const protocol::RuntimeEnvelope& envelope)
{
    return impl_->send(envelope);
}

std::optional<protocol::RuntimeEnvelope>
HostProcess::receive(std::chrono::milliseconds timeout)
{
    return impl_->receive(timeout);
}

std::optional<int>
HostProcess::wait(std::chrono::milliseconds timeout)
{
    return impl_->wait(timeout);
}

bool
HostProcess::terminate()
{
    return impl_->terminate();
}

bool
HostProcess::running() const
{
    return impl_->running();
}

} // namespace celestia::runtime::process

#endif // !_WIN32
