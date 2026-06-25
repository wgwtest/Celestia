#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <celruntime/process/processsupervisor.h>
#include <celruntime/runtimeconfig.h>

namespace
{

std::filesystem::path
sourceRoot()
{
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
}

std::filesystem::path
buildRoot()
{
    return std::filesystem::current_path().parent_path().parent_path();
}

std::string
readSourceFile(std::string_view relativePath)
{
    std::ifstream input(sourceRoot() / std::filesystem::path(relativePath));
    REQUIRE(input.good());

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool
contains(std::string_view text, std::string_view token)
{
    return text.find(token) != std::string_view::npos;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step7 SDL runtime");

TEST_CASE("RuntimeConfig accepts Step7 host transport names")
{
    celestia::runtime::RuntimeConfig config;
    CHECK(config.hostTransport() == "stdio-pipe");

    CHECK_FALSE(celestia::runtime::applyRuntimeConfigArgument(config, "--host-transport=stdio-files"));
    CHECK(config.hostTransport() == "stdio-pipe");

    CHECK(celestia::runtime::applyRuntimeConfigArgument(config, "--host-transport=stdio"));
    CHECK(config.hostTransport() == "stdio-pipe");

    CHECK(celestia::runtime::applyRuntimeConfigArgument(config, "--host-transport=stdio-pipe"));
    CHECK(config.hostTransport() == "stdio-pipe");
    CHECK_FALSE(celestia::runtime::applyRuntimeConfigArgument(config, "--host-transport=tcp"));
}

TEST_CASE("ProcessSupervisor runs live runtime through stdio-pipe")
{
#ifndef _WIN32
    MESSAGE("Step7 live ProcessSupervisor runtime is Windows-only in first implementation");
    return;
#else
    celestia::runtime::process::ProcessSupervisorOptions options;
    options.runtimeHostDirectory = buildRoot() / "src" / "celruntime";
    options.viewId = std::string(celestia::runtime::RuntimeConfig::Debug2DViewId);
    options.durationMilliseconds = 300;
    options.hostTransport = "stdio-pipe";
    options.sessionId = "step7-process-supervisor-live-test";

    celestia::runtime::process::ProcessSupervisor supervisor(options);
    const auto result = supervisor.runRuntime();

    CAPTURE(result.log);
    CHECK(result.success);
    CHECK(result.controllerReady);
    CHECK(result.modelReady);
    CHECK(result.viewReady);
    CHECK(result.viewFrameCount >= 2);
    CHECK(contains(result.log, "all hosts stopped"));
#endif
}

TEST_CASE("SDL multi-process serve uses live runtime without file fallback")
{
    const auto runtimeHeader = readSourceFile("src/celruntime/runtimeconfig.h");
    const auto runtimeSource = readSourceFile("src/celruntime/runtimeconfig.cpp");
    const auto supervisorHeader = readSourceFile("src/celruntime/process/processsupervisor.h");
    const auto sdlMain = readSourceFile("src/celestia/sdl/sdlmain.cpp");

    CHECK(contains(runtimeHeader, "setHostTransport"));
    CHECK(contains(runtimeSource, "stdio-pipe"));
    CHECK(contains(supervisorHeader, "runRuntime() const"));
    CHECK(contains(sdlMain, "runRuntime()"));
    CHECK(contains(sdlMain, "sdl-step8-serve"));
    CHECK_FALSE(contains(runtimeSource, "stdio-files"));
    CHECK_FALSE(contains(supervisorHeader, "runServeSmoke"));
    CHECK_FALSE(contains(sdlMain, "stdio-files"));
    CHECK_FALSE(contains(sdlMain, "sdl-step6-serve"));
    CHECK_FALSE(contains(sdlMain, "multi-process serve currently supports"));
}

TEST_SUITE_END();
