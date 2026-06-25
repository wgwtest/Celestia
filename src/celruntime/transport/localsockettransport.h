// localsockettransport.h
//
// Copyright (C) 2026, the Celestia Development Team

#pragma once

#include "framedtransport.h"
#include "runtimetransport.h"

#include <memory>
#include <string_view>

#include <celruntime/process/hostprocess.h>

namespace celestia::runtime::transport
{

class LocalSocketTransport : public RuntimeTransport
{
public:
    explicit LocalSocketTransport(process::HostProcessOptions);
    ~LocalSocketTransport() override;

    LocalSocketTransport(const LocalSocketTransport&) = delete;
    LocalSocketTransport& operator=(const LocalSocketTransport&) = delete;

    std::string_view kind() const override;
    bool open(std::string* error = nullptr) override;
    bool send(const protocol::RuntimeEnvelope&) override;
    std::optional<protocol::RuntimeEnvelope> receive(std::chrono::milliseconds timeout) override;
    std::optional<int> wait(std::chrono::milliseconds timeout) override;
    bool terminate() override;
    bool running() const override;
    void close() override;
    RuntimeTransportStats stats() const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

bool localSocketTransportSupported();
std::unique_ptr<FramedTransport> connectLocalSocketEndpoint(std::string_view endpoint,
                                                            std::string* error = nullptr);

} // namespace celestia::runtime::transport
