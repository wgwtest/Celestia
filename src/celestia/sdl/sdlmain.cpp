// sdlmain.cpp
//
// Copyright (C) 2020-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <array>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <celestia/celestiacore.h>
#include <celruntime/assembly/runtimeassemblyrunner.h>
#include <celruntime/ipc/message.h>
#include <celruntime/process/processsupervisor.h>
#include <celruntime/runtimeconfig.h>
#include <celruntime/viewplugin/viewpluginregistry.h>
#include <celutil/gettext.h>
#include "alerter.h"
#include "appwindow.h"
#include "environment.h"
#include "settings.h"

#ifdef _WIN32
#include <fmt/format.h>
#include <windows.h>
#endif

namespace
{

std::filesystem::path
getDataDir()
{
#ifdef _WIN32
    fmt::basic_memory_buffer<wchar_t, MAX_PATH + 1> buffer;
    buffer.resize(MAX_PATH + 1);

    std::size_t size;
    errno_t result;
    while ((result = _wgetenv_s(&size, buffer.data(), buffer.size(), L"CELESTIA_DATA_DIR")) == ERANGE)
        buffer.resize(size);
    if (result == 0 && size > 0)
        return buffer.data();
#else
    if (const char* dataDirEnv = std::getenv("CELESTIA_DATA_DIR"); dataDirEnv)
        return dataDirEnv;
#endif

    return CONFIG_DATA_DIR;
}

bool
writeText(const std::filesystem::path& path, std::string_view text)
{
    std::ofstream output(path);
    if (!output.good())
        return false;

    output << text;
    return true;
}

std::optional<std::string>
readText(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input.good())
        return std::nullopt;

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string
quotePath(const std::filesystem::path& path)
{
    return "\"" + path.string() + "\"";
}

std::filesystem::path
runtimeHostDirectory(char* executablePath)
{
    return std::filesystem::absolute(executablePath)
        .parent_path()
        .parent_path()
        .parent_path() / "celruntime";
}

bool
runRuntimeHostHandshake(const std::filesystem::path& runtimeHostDir,
                        std::string_view hostName,
                        const celestia::runtime::RuntimeConfig& runtimeConfig)
{
#ifdef _WIN32
    const auto executable = runtimeHostDir / (std::string(hostName) + ".exe");
#else
    const auto executable = runtimeHostDir / std::string(hostName);
#endif
    if (!std::filesystem::exists(executable))
        return false;

    const auto inputPath = std::filesystem::temp_directory_path() /
        ("celestia-sdl-" + std::string(hostName) + ".in");
    const auto outputPath = std::filesystem::temp_directory_path() /
        ("celestia-sdl-" + std::string(hostName) + ".out");
    const auto scriptPath = std::filesystem::temp_directory_path() /
        ("celestia-sdl-" + std::string(hostName) + ".cmd");

    const auto request = celestia::runtime::ipc::RuntimeMessage::command("runtime.handshake", "sdl");
    if (!writeText(inputPath, celestia::runtime::ipc::serializeMessage(request)))
        return false;

    std::filesystem::remove(outputPath);

#ifdef _WIN32
    if (!writeText(scriptPath,
                   "@echo off\n" +
                       quotePath(executable) +
                       " --stdio --protocol-version=1 --view=" +
                       runtimeConfig.selectedViewId() +
                       " --once < " + quotePath(inputPath) + " > " + quotePath(outputPath) + "\n" +
                       "exit /b %ERRORLEVEL%\n"))
    {
        return false;
    }

    const auto command = "cmd.exe /d /c call " + quotePath(scriptPath);
#else
    const auto command =
        quotePath(executable) +
        " --stdio --protocol-version=1 --view=" +
        runtimeConfig.selectedViewId() +
        " --once < " + quotePath(inputPath) + " > " + quotePath(outputPath);
#endif

    if (std::system(command.c_str()) != 0)
        return false;

    const auto responseText = readText(outputPath);
    if (!responseText.has_value())
        return false;

    const auto response = celestia::runtime::ipc::deserializeMessage(*responseText);
    return response.has_value() &&
           response->kind == celestia::runtime::ipc::MessageKind::Event &&
           response->name == "runtime.ack";
}

bool
runMultiProcessOnce(char* executablePath, const celestia::runtime::RuntimeConfig& runtimeConfig)
{
    if (!runtimeConfig.runOnce())
        return false;

    if (runtimeConfig.selectedViewId() != celestia::runtime::RuntimeConfig::Debug2DViewId)
        return false;

    const auto runtimeHostDir = runtimeHostDirectory(executablePath);
    constexpr std::array<std::string_view, 3> hosts = {
        "celestia-model-host",
        "celestia-controller-host",
        "celestia-view-host",
    };

    for (const auto host : hosts)
    {
        if (!runRuntimeHostHandshake(runtimeHostDir, host, runtimeConfig))
            return false;
    }

    return true;
}

bool
runMultiProcessServe(char* executablePath, const celestia::runtime::RuntimeConfig& runtimeConfig)
{
    if (!runtimeConfig.serve())
        return false;

    celestia::runtime::process::ProcessSupervisorOptions options;
    options.runtimeHostDirectory = runtimeHostDirectory(executablePath);
    options.viewId = runtimeConfig.selectedViewId();
    options.durationMilliseconds = runtimeConfig.durationMilliseconds();
    options.hostTransport = std::string(runtimeConfig.hostTransport());
    options.switchViewAfterMilliseconds = runtimeConfig.switchViewAfterMilliseconds();
    options.switchViewId = runtimeConfig.switchViewId();
    if (options.viewId == celestia::runtime::RuntimeConfig::DefaultViewId)
        options.sessionId = "sdl-step8-serve";
    else
        options.sessionId = options.hostTransport == "stdio-files" ? "sdl-step6-serve" : "sdl-step7-serve";

    celestia::runtime::process::ProcessSupervisor supervisor(options);
    const auto result = options.hostTransport == "stdio-files"
        ? supervisor.runServeSmoke()
        : supervisor.runRuntime();
    std::cout << result.log;
    return result.success;
}

bool
runRuntimeAssemblyConfig(char* executablePath, const celestia::runtime::RuntimeConfig& runtimeConfig)
{
    std::string error;
    auto config = celestia::runtime::assembly::loadRuntimeAssemblyConfig(
        runtimeConfig.runtimeConfigPath(),
        runtimeHostDirectory(executablePath),
        getDataDir(),
        &error);
    if (!config.has_value())
    {
        std::cerr << error << '\n';
        return false;
    }

    celestia::runtime::assembly::RuntimeAssemblyRunner runner(std::move(*config));
    const auto result = runner.run();
    std::cout << result.log;
    return result.success;
}

bool
listRuntimeViews(const celestia::runtime::RuntimeConfig& runtimeConfig)
{
    auto registry = celestia::runtime::viewplugin::builtinViewPluginRegistry();
    if (!runtimeConfig.pluginDirectory().empty())
    {
        std::string error;
        if (!registry.discover(runtimeConfig.pluginDirectory(), &error))
        {
            std::cerr << error << '\n';
            return false;
        }
    }

    for (const auto& id : registry.viewIds())
        std::cout << id << '\n';
    return true;
}

bool
parseRuntimeConfig(int argc, char** argv, celestia::runtime::RuntimeConfig& runtimeConfig)
{
    constexpr std::string_view viewOption{ "--view=" };
    constexpr std::string_view modeOption{ "--mvc-mode=" };
    constexpr std::string_view durationOption{ "--duration-ms=" };
    constexpr std::string_view transportOption{ "--host-transport=" };
    constexpr std::string_view pluginDirOption{ "--plugin-dir=" };
    constexpr std::string_view switchAfterOption{ "--switch-view-after-ms=" };
    constexpr std::string_view switchViewOption{ "--switch-view=" };
    constexpr std::string_view runtimeConfigOption{ "--runtime-config=" };

    for (int i = 1; i < argc; ++i)
    {
        std::string_view argument{ argv[i] != nullptr ? argv[i] : "" };
        if (argument == "--runtime-config")
        {
            if (i + 1 >= argc || argv[i + 1] == nullptr)
                return false;

            runtimeConfig.setRuntimeConfigPath(argv[++i]);
            continue;
        }

        if ((argument.compare(0, viewOption.size(), viewOption) == 0 ||
             argument.compare(0, modeOption.size(), modeOption) == 0 ||
             argument.compare(0, durationOption.size(), durationOption) == 0 ||
             argument.compare(0, transportOption.size(), transportOption) == 0 ||
             argument.compare(0, pluginDirOption.size(), pluginDirOption) == 0 ||
             argument.compare(0, switchAfterOption.size(), switchAfterOption) == 0 ||
             argument.compare(0, switchViewOption.size(), switchViewOption) == 0 ||
             argument.compare(0, runtimeConfigOption.size(), runtimeConfigOption) == 0 ||
             argument == "--once" ||
             argument == "--serve" ||
             argument == "--list-views") &&
            !celestia::runtime::applyRuntimeConfigArgument(runtimeConfig, argument))
        {
            return false;
        }
    }

    return true;
}

}

int
main(int argc, char **argv)
{
    using celestia::sdl::Environment;
    using celestia::sdl::Settings;

    CelestiaCore::initLocale();

#ifdef ENABLE_NLS
    bindtextdomain("celestia", LOCALEDIR);
    bind_textdomain_codeset("celestia", "UTF-8");
    bindtextdomain("celestia-data", LOCALEDIR);
    bind_textdomain_codeset("celestia", "UTF-8");
    textdomain("celestia");
#endif

    auto environment = Environment::init();
    if (!environment)
        return EXIT_FAILURE;

    std::filesystem::path dataDir = getDataDir();
    celestia::runtime::RuntimeConfig runtimeConfig;
    if (!parseRuntimeConfig(argc, argv, runtimeConfig))
        return EXIT_FAILURE;

    if (runtimeConfig.listViews())
        return listRuntimeViews(runtimeConfig) ? EXIT_SUCCESS : EXIT_FAILURE;

    if (!runtimeConfig.runtimeConfigPath().empty())
        return runRuntimeAssemblyConfig(argv[0], runtimeConfig) ? EXIT_SUCCESS : EXIT_FAILURE;

    if (runtimeConfig.runtimeMode() == celestia::runtime::RuntimeMode::MultiProcess)
    {
        if (runtimeConfig.runOnce())
            return runMultiProcessOnce(argv[0], runtimeConfig) ? EXIT_SUCCESS : EXIT_FAILURE;

        if (runtimeConfig.serve())
            return runMultiProcessServe(argv[0], runtimeConfig) ? EXIT_SUCCESS : EXIT_FAILURE;

        return EXIT_FAILURE;
    }

    std::error_code ec;
    std::filesystem::current_path(dataDir, ec);
    if (ec)
        return EXIT_FAILURE;

    auto celestiaCore = std::make_unique<CelestiaCore>();
    auto alerter = std::make_unique<celestia::sdl::Alerter>();
    celestiaCore->setAlerter(alerter.get());

    if (!celestiaCore->initSimulation())
        return EXIT_FAILURE;

    if (!environment->setGLAttributes(celestiaCore->getConfig()->renderDetails.aaSamples))
        return EXIT_FAILURE;

    Settings settings = Settings::load(environment->getSettingsPath());

    auto window = environment->createAppWindow(settings, std::move(celestiaCore), std::move(alerter));
    if (!window)
        return EXIT_FAILURE;

    window->dumpGLInfo();
    return window->run(settings, runtimeConfig) ? EXIT_SUCCESS : EXIT_FAILURE;
}
