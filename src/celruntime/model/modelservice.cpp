// modelservice.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "modelservice.h"

#include <array>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <celruntime/protocol/lifecycle.h>
#include <celruntime/viewframecodec.h>

namespace celestia::runtime::model
{
namespace
{

using protocol::RuntimeEnvelope;
using protocol::RuntimeMessageKind;
using protocol::RuntimeRole;

constexpr std::array<char, 16> HexDigits{
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
};

bool
isUnreserved(char c)
{
    return std::isalnum(static_cast<unsigned char>(c)) != 0 ||
           c == '-' || c == '_' || c == '.' || c == '~';
}

std::string
escape(std::string_view text)
{
    std::string output;
    for (const auto c : text)
    {
        if (isUnreserved(c))
        {
            output.push_back(c);
            continue;
        }

        const auto value = static_cast<unsigned char>(c);
        output.push_back('%');
        output.push_back(HexDigits[(value >> 4U) & 0x0FU]);
        output.push_back(HexDigits[value & 0x0FU]);
    }

    return output;
}

int
hexValue(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

std::optional<std::string>
unescape(std::string_view text)
{
    std::string output;
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] != '%')
        {
            output.push_back(text[i]);
            continue;
        }

        if (i + 2 >= text.size())
            return std::nullopt;

        const auto high = hexValue(text[i + 1]);
        const auto low = hexValue(text[i + 2]);
        if (high < 0 || low < 0)
            return std::nullopt;

        output.push_back(static_cast<char>((high << 4U) | low));
        i += 2;
    }

    return output;
}

std::unordered_map<std::string, std::string>
parsePayload(std::string_view payload)
{
    std::unordered_map<std::string, std::string> fields;
    std::size_t offset = 0;

    while (offset <= payload.size())
    {
        auto end = payload.find(';', offset);
        if (end == std::string_view::npos)
            end = payload.size();

        const auto part = payload.substr(offset, end - offset);
        const auto separator = part.find('=');
        if (separator != std::string_view::npos)
        {
            const auto value = unescape(part.substr(separator + 1));
            if (value.has_value())
                fields[std::string(part.substr(0, separator))] = *value;
        }

        if (end == payload.size())
            break;
        offset = end + 1;
    }

    return fields;
}

std::optional<double>
parseDouble(std::string_view text)
{
    try
    {
        std::size_t consumed = 0;
        const auto value = std::stod(std::string(text), &consumed);
        if (consumed != text.size())
            return std::nullopt;
        return value;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::uint64_t>
parseUint64(std::string_view text)
{
    try
    {
        std::size_t consumed = 0;
        const auto value = std::stoull(std::string(text), &consumed);
        if (consumed != text.size())
            return std::nullopt;
        return static_cast<std::uint64_t>(value);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

bool
parseBool(std::string_view text)
{
    return text == "true" || text == "1" || text == "yes";
}

std::string
formatDouble(double value)
{
    std::ostringstream output;
    output << std::setprecision(17) << value;
    return output.str();
}

class SyntheticSimulationBackend final : public SimulationBackend
{
public:
    bool load(const RuntimeDataPaths&) override
    {
        loaded_ = true;
        return true;
    }

    void setTime(double time) override
    {
        time_ = time;
    }

    void step(double dt) override
    {
        if (!loaded_)
            load({});

        time_ += dt;
        ++frameId_;
    }

    ViewFrame snapshot() const override
    {
        ViewFrame frame;
        frame.frameId = frameId_;
        frame.time = time_;
        frame.summary = "synthetic model frame " + std::to_string(frameId_);

        ViewFrameSelection earth;
        earth.type = "body";
        earth.id = "Earth";
        earth.positionKm = { time_, time_ * 0.5, 0.0 };
        earth.visible = true;
        earth.clickable = true;
        frame.selections.push_back(std::move(earth));
        return frame;
    }

private:
    bool loaded_{ false };
    double time_{ 0.0 };
    std::uint64_t frameId_{ 0 };
};

RuntimeEnvelope
makeResponse(const RuntimeEnvelope& request,
             RuntimeMessageKind kind,
             std::string name,
             std::string payload = {})
{
    RuntimeEnvelope response;
    response.sessionId = request.sessionId;
    response.sequenceId = request.sequenceId;
    response.sourceRole = RuntimeRole::Model;
    response.targetRole = request.sourceRole;
    response.kind = kind;
    response.name = std::move(name);
    response.payload = std::move(payload);
    return response;
}

} // end unnamed namespace

std::string
serializeViewFrame(const ViewFrame& frame)
{
    return celestia::runtime::serializeViewFrame(frame);
}

std::optional<ViewFrame>
deserializeViewFrame(std::string_view payload)
{
    return celestia::runtime::deserializeViewFrame(payload);
}

ModelService::ModelService(std::string sessionId)
    : ModelService(std::move(sessionId), std::make_unique<SyntheticSimulationBackend>())
{
}

ModelService::ModelService(std::string sessionId, std::unique_ptr<SimulationBackend> backend)
    : sessionId_(std::move(sessionId))
    , backend_(std::move(backend))
{
    if (backend_ != nullptr)
        backend_->load({});
}

ModelService::~ModelService() = default;
ModelService::ModelService(ModelService&&) noexcept = default;
ModelService& ModelService::operator=(ModelService&&) noexcept = default;

bool
ModelService::isRunning() const
{
    return running_;
}

bool
ModelService::isPaused() const
{
    return paused_;
}

double
ModelService::simulationTime() const
{
    return backend_ == nullptr ? 0.0 : backend_->snapshot().time;
}

RuntimeEnvelope
ModelService::lifecycleResponse(const RuntimeEnvelope& request, std::string name) const
{
    return makeResponse(request, RuntimeMessageKind::Lifecycle, std::move(name));
}

RuntimeEnvelope
ModelService::errorResponse(const RuntimeEnvelope& request, std::string message) const
{
    return makeResponse(request, RuntimeMessageKind::Error, protocol::RuntimeError, std::move(message));
}

RuntimeEnvelope
ModelService::viewFrameResponse(const RuntimeEnvelope& request) const
{
    return makeResponse(request, RuntimeMessageKind::ViewFrame, "view.frame",
                        backend_ == nullptr
                            ? std::string{}
                            : celestia::runtime::model::serializeViewFrame(backend_->snapshot()));
}

RuntimeEnvelope
ModelService::handle(const RuntimeEnvelope& request)
{
    if (request.kind == RuntimeMessageKind::Lifecycle)
    {
        if (request.name == protocol::RuntimeStart)
        {
            running_ = true;
            return lifecycleResponse(request, "runtime.started");
        }

        if (request.name == protocol::RuntimeShutdown)
        {
            running_ = false;
            return lifecycleResponse(request, protocol::RuntimeStopped);
        }

        return errorResponse(request, "unknown lifecycle message: " + request.name);
    }

    if (request.kind != RuntimeMessageKind::Command)
        return errorResponse(request, "model service expects command messages");

    const auto payload = parsePayload(request.payload);
    if (request.name == "model.setTime")
    {
        const auto value = payload.find("time");
        if (value == payload.end())
            return errorResponse(request, "model.setTime requires time");

        const auto parsed = parseDouble(value->second);
        if (!parsed.has_value())
            return errorResponse(request, "invalid model.setTime value");

        if (backend_ != nullptr)
            backend_->setTime(*parsed);
        return viewFrameResponse(request);
    }

    if (request.name == "model.setTimeScale")
    {
        const auto value = payload.find("timeScale");
        if (value == payload.end())
            return errorResponse(request, "model.setTimeScale requires timeScale");

        const auto parsed = parseDouble(value->second);
        if (!parsed.has_value())
            return errorResponse(request, "invalid model.setTimeScale value");

        timeScale_ = *parsed;
        return viewFrameResponse(request);
    }

    if (request.name == "model.setPaused")
    {
        const auto value = payload.find("paused");
        if (value == payload.end())
            return errorResponse(request, "model.setPaused requires paused");

        paused_ = parseBool(value->second);
        return viewFrameResponse(request);
    }

    if (request.name == "model.step")
    {
        const auto value = payload.find("dt");
        const auto dt = value == payload.end() ? std::optional<double>{ 1.0 } : parseDouble(value->second);
        if (!dt.has_value())
            return errorResponse(request, "invalid model.step dt");

        if (running_ && !paused_ && backend_ != nullptr)
            backend_->step(*dt * timeScale_);
        return viewFrameResponse(request);
    }

    if (request.name == "model.requestSnapshot")
        return viewFrameResponse(request);

    return errorResponse(request, "unknown model command: " + request.name);
}

} // namespace celestia::runtime::model
