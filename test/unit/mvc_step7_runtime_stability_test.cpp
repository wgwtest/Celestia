#include <doctest.h>

#include <filesystem>
#include <string>
#include <string_view>

#include <celruntime/process/runtimesession.h>
#include <celruntime/runtimeconfig.h>

namespace
{

std::filesystem::path
buildRoot()
{
    return std::filesystem::current_path().parent_path().parent_path();
}

bool
contains(std::string_view text, std::string_view token)
{
    return text.find(token) != std::string_view::npos;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step7 runtime stability");

TEST_CASE("RuntimeSession sustains a multi-second MVC runtime")
{
#ifndef _WIN32
    MESSAGE("Step7 live RuntimeSession is Windows-only in first implementation");
    return;
#else
    celestia::runtime::process::RuntimeSessionOptions options;
    options.runtimeHostDirectory = buildRoot() / "src" / "celruntime";
    options.viewId = std::string(celestia::runtime::RuntimeConfig::Debug2DViewId);
    options.sessionId = "step7-runtime-stability-test";
    options.durationMilliseconds = 5000;
    options.tickMilliseconds = 125;
    options.hostReadyTimeoutMilliseconds = 3000;
    options.shutdownTimeoutMilliseconds = 3000;
    options.heartbeatEveryTicks = 10;

    celestia::runtime::process::RuntimeSession session(options);
    const auto result = session.run();

    CAPTURE(result.log);
    CHECK(result.success);
    CHECK(result.tickCount >= 30);
    CHECK(result.viewFrameCount >= 30);
    CHECK(result.controllerReady);
    CHECK(result.modelReady);
    CHECK(result.viewReady);
    CHECK(result.controllerStopped);
    CHECK(result.modelStopped);
    CHECK(result.viewStopped);
    CHECK(result.controllerExitCode == 0);
    CHECK(result.modelExitCode == 0);
    CHECK(result.viewExitCode == 0);
    CHECK(contains(result.log, "all hosts stopped"));
#endif
}

TEST_SUITE_END();
