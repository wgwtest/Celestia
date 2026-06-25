// runtimetransport.h
//
// Copyright (C) 2026, the Celestia Development Team

#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include <celruntime/protocol/envelope.h>

namespace celestia::runtime::transport
{

struct RuntimeTransportStats
{
    int sentMessages{ 0 };
    int receivedMessages{ 0 };
    int errors{ 0 };
    std::size_t sentBytes{ 0 };
    std::size_t receivedBytes{ 0 };
};

class RuntimeTransport
{
public:
    virtual ~RuntimeTransport() = default;

    virtual std::string_view kind() const = 0;
    virtual bool open(std::string* error = nullptr) = 0;
    virtual bool send(const protocol::RuntimeEnvelope&) = 0;
    virtual std::optional<protocol::RuntimeEnvelope> receive(std::chrono::milliseconds timeout) = 0;
    virtual std::optional<int> wait(std::chrono::milliseconds timeout) = 0;
    virtual bool terminate() = 0;
    virtual bool running() const = 0;
    virtual void close() = 0;
    virtual RuntimeTransportStats stats() const = 0;
};

} // namespace celestia::runtime::transport
