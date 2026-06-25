// stdiopipetransport.cpp
//
// Copyright (C) 2026, the Celestia Development Team

#include "stdiopipetransport.h"

#include <utility>

namespace celestia::runtime::transport
{

StdioPipeTransport::StdioPipeTransport(process::HostProcessOptions options)
    : options_(std::move(options))
{
}

StdioPipeTransport::~StdioPipeTransport() = default;

std::string_view
StdioPipeTransport::kind() const
{
    return "stdio-pipe";
}

bool
StdioPipeTransport::open(std::string* error)
{
    auto options = options_;
    options.arguments.insert(options.arguments.begin(), "--stdio");
    process_ = std::make_unique<process::HostProcess>(std::move(options));
    if (!process_->start())
    {
        if (error != nullptr)
            *error = "failed to start stdio-pipe host";
        ++stats_.errors;
        return false;
    }
    return true;
}

bool
StdioPipeTransport::send(const protocol::RuntimeEnvelope& envelope)
{
    if (process_ == nullptr || !process_->send(envelope))
    {
        ++stats_.errors;
        return false;
    }
    ++stats_.sentMessages;
    return true;
}

std::optional<protocol::RuntimeEnvelope>
StdioPipeTransport::receive(std::chrono::milliseconds timeout)
{
    if (process_ == nullptr)
        return std::nullopt;
    auto message = process_->receive(timeout);
    if (message.has_value())
        ++stats_.receivedMessages;
    else
        ++stats_.errors;
    return message;
}

std::optional<int>
StdioPipeTransport::wait(std::chrono::milliseconds timeout)
{
    return process_ == nullptr ? std::nullopt : process_->wait(timeout);
}

bool
StdioPipeTransport::terminate()
{
    return process_ != nullptr && process_->terminate();
}

bool
StdioPipeTransport::running() const
{
    return process_ != nullptr && process_->running();
}

void
StdioPipeTransport::close()
{
    if (process_ != nullptr && process_->running())
        process_->terminate();
}

RuntimeTransportStats
StdioPipeTransport::stats() const
{
    return stats_;
}

} // namespace celestia::runtime::transport
