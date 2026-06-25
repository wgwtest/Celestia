// runtimeassemblyconfig.cpp
//
// Copyright (C) 2026, the Celestia Development Team

#include "runtimeassemblyconfig.h"

#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

#include <celruntime/transport/transportfactory.h>

namespace celestia::runtime::assembly
{
namespace
{

void
setError(std::string* error, std::string message)
{
    if (error != nullptr)
        *error = std::move(message);
}

std::string
viewHostName(std::string_view viewId)
{
    return viewId == RuntimeConfig::DefaultViewId ||
           viewId == "celestia.view3d.opengl"
        ? "celestia-view3d-host"
        : "celestia-view-host";
}

std::string_view
trim(std::string_view text)
{
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t' || text.front() == '\r'))
        text.remove_prefix(1);
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r'))
        text.remove_suffix(1);
    return text;
}

std::optional<int>
parseInt(std::string_view text)
{
    try
    {
        std::size_t consumed = 0;
        const auto value = std::stoi(std::string(text), &consumed);
        if (consumed != text.size())
            return std::nullopt;
        return value;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

int
indentWidth(std::string_view line)
{
    int width = 0;
    for (const auto ch : line)
    {
        if (ch == ' ')
            ++width;
        else
            break;
    }
    return width;
}

std::filesystem::path
resolvePath(const std::filesystem::path& baseDirectory, std::string_view text)
{
    std::filesystem::path path{ std::string(text) };
    if (path.is_absolute())
        return path;
    return baseDirectory / path;
}

} // end unnamed namespace

RuntimeAssemblyConfig
RuntimeAssemblyConfig::fromRuntimeConfig(const RuntimeConfig& runtimeConfig,
                                         std::filesystem::path runtimeHostDirectory,
                                         std::filesystem::path contentRoot,
                                         std::string sessionId)
{
    RuntimeAssemblyConfig config;
    config.session.id = std::move(sessionId);
    config.session.durationMilliseconds = runtimeConfig.durationMilliseconds();
    config.resources.contentRoot = std::move(contentRoot);
    config.runtimeHostDirectory = std::move(runtimeHostDirectory);
    config.transport.controlKind = std::string(runtimeConfig.hostTransport());
    config.transport.dataKind = "framed-envelope";
    config.view.id = runtimeConfig.selectedViewId();
    config.view.pluginDirectory = runtimeConfig.pluginDirectory();
    config.view.hostName = viewHostName(config.view.id);
    config.view.switchAfterMilliseconds = runtimeConfig.switchViewAfterMilliseconds();
    config.view.switchViewId = runtimeConfig.switchViewId();
    return config;
}

bool
RuntimeAssemblyConfig::validate(const viewplugin::ViewPluginRegistry& registry, std::string* error) const
{
    if (session.id.empty())
    {
        setError(error, "session id is required");
        return false;
    }

    if (resources.contentRoot.empty())
    {
        setError(error, "contentRoot is required");
        return false;
    }

    if (runtimeHostDirectory.empty())
    {
        setError(error, "runtime host directory is required");
        return false;
    }

    if (!transport::isSupportedRuntimeTransport(transport.controlKind))
    {
        setError(error, "unsupported transport: " + transport.controlKind);
        return false;
    }

    if (transport.dataKind.empty())
    {
        setError(error, "data transport kind is required");
        return false;
    }

    if (registry.find(view.id) == nullptr)
    {
        setError(error, "unknown view id: " + view.id);
        return false;
    }

    return true;
}

std::optional<RuntimeAssemblyConfig>
loadRuntimeAssemblyConfig(const std::filesystem::path& path,
                          std::filesystem::path runtimeHostDirectory,
                          std::filesystem::path defaultContentRoot,
                          std::string* error)
{
    std::ifstream input(path);
    if (!input.good())
    {
        setError(error, "runtime config file does not exist: " + path.string());
        return std::nullopt;
    }

    RuntimeAssemblyConfig config;
    config.runtimeHostDirectory = std::move(runtimeHostDirectory);
    config.resources.contentRoot = std::move(defaultContentRoot);
    config.view.hostName = viewHostName(config.view.id);

    const auto baseDirectory = path.parent_path();
    std::vector<std::string> sections;
    std::string line;
    while (std::getline(input, line))
    {
        auto text = std::string_view(line);
        const auto comment = text.find('#');
        if (comment != std::string_view::npos)
            text = text.substr(0, comment);
        if (trim(text).empty())
            continue;

        const auto indent = indentWidth(text);
        const auto depth = indent / 2;
        text = trim(text);

        if (text.back() == ':')
        {
            if (sections.size() > static_cast<std::size_t>(depth))
                sections.resize(static_cast<std::size_t>(depth));
            sections.push_back(std::string(trim(text.substr(0, text.size() - 1))));
            continue;
        }

        const auto colon = text.find(':');
        if (colon == std::string_view::npos)
            continue;

        if (sections.size() > static_cast<std::size_t>(depth))
            sections.resize(static_cast<std::size_t>(depth));

        const auto key = trim(text.substr(0, colon));
        auto value = trim(text.substr(colon + 1));
        if (value.size() >= 2 &&
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\'')))
        {
            value.remove_prefix(1);
            value.remove_suffix(1);
        }

        const auto section = sections.empty() ? std::string{} : sections.back();
        const auto parent = sections.size() >= 2 ? sections[sections.size() - 2] : std::string{};

        if (section == "session")
        {
            if (key == "id")
                config.session.id = std::string(value);
            else if (key == "durationMs")
                config.session.durationMilliseconds = parseInt(value).value_or(config.session.durationMilliseconds);
            else if (key == "tickMs")
                config.session.tickMilliseconds = parseInt(value).value_or(config.session.tickMilliseconds);
            else if (key == "heartbeatMs")
                config.session.heartbeatMilliseconds = parseInt(value).value_or(config.session.heartbeatMilliseconds);
            else if (key == "shutdownTimeoutMs")
                config.session.shutdownTimeoutMilliseconds = parseInt(value).value_or(config.session.shutdownTimeoutMilliseconds);
            else if (key == "readyTimeoutMs")
                config.session.readyTimeoutMilliseconds = parseInt(value).value_or(config.session.readyTimeoutMilliseconds);
        }
        else if (section == "resources")
        {
            if (key == "contentRoot")
                config.resources.contentRoot = resolvePath(baseDirectory, value);
        }
        else if (section == "control" && parent == "transport")
        {
            if (key == "kind")
                config.transport.controlKind = std::string(value);
        }
        else if (section == "data" && parent == "transport")
        {
            if (key == "kind")
                config.transport.dataKind = std::string(value);
        }
        else if (section == "model")
        {
            if (key == "host")
                config.model.hostName = std::string(value);
        }
        else if (section == "controller")
        {
            if (key == "host")
                config.controller.hostName = std::string(value);
        }
        else if (section == "view")
        {
            if (key == "id")
            {
                config.view.id = std::string(value);
                config.view.hostName = viewHostName(config.view.id);
            }
            else if (key == "pluginDir" || key == "pluginDirectory")
                config.view.pluginDirectory = std::string(value);
            else if (key == "host")
                config.view.hostName = std::string(value);
            else if (key == "switchAfterMs")
                config.view.switchAfterMilliseconds = parseInt(value).value_or(0);
            else if (key == "switchView")
                config.view.switchViewId = std::string(value);
        }
        else if (section == "supervisor")
        {
            if (key == "traceFile")
                config.supervisor.traceFile = resolvePath(baseDirectory, value);
            else if (key == "restartPolicy")
                config.supervisor.restartPolicy = std::string(value);
            else if (key == "maxRestarts")
                config.supervisor.maxRestarts = parseInt(value).value_or(0);
        }
    }

    return config;
}

} // namespace celestia::runtime::assembly
