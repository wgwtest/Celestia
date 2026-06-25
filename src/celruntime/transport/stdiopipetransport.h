// stdiopipetransport.h
//
// Copyright (C) 2026, the Celestia Development Team

#pragma once

#include "runtimetransport.h"

#include <memory>

#include <celruntime/process/hostprocess.h>

namespace celestia::runtime::transport
{

class StdioPipeTransport : public RuntimeTransport
{
public:
    explicit StdioPipeTransport(process::HostProcessOptions);
    ~StdioPipeTransport() override;

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
    process::HostProcessOptions options_;
    std::unique_ptr<process::HostProcess> process_;
    RuntimeTransportStats stats_;
};

} // namespace celestia::runtime::transport
