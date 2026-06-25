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

celestia::runtime::process::RuntimeSessionOptions
baseOptions(std::string sessionId)
{
    celestia::runtime::process::RuntimeSessionOptions options;
    options.runtimeHostDirectory = buildRoot() / "src" / "celruntime";
    options.viewId = std::string(celestia::runtime::RuntimeConfig::Debug2DViewId);
    options.sessionId = std::move(sessionId);
    options.durationMilliseconds = 80;
    options.tickMilliseconds = 10;
    options.hostReadyTimeoutMilliseconds = 3000;
    options.shutdownTimeoutMilliseconds = 3000;
    return options;
}

bool
contains(std::string_view text, std::string_view token)
{
    return text.find(token) != std::string_view::npos;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step7 supervisor resilience");

TEST_CASE("RuntimeSession reports host ready timeout")
{
#ifndef _WIN32
    MESSAGE("Step7 live RuntimeSession is Windows-only in first implementation");
    return;
#else
    auto options = baseOptions("step7-ready-timeout-test");
    options.hostReadyTimeoutMilliseconds = 0;

    celestia::runtime::process::RuntimeSession session(options);
    const auto result = session.run();

    CAPTURE(result.log);
    CHECK_FALSE(result.success);
    CHECK(contains(result.log, "receive timeout"));
#endif
}

TEST_CASE("RuntimeSession records host runtime errors")
{
#ifndef _WIN32
    MESSAGE("Step7 live RuntimeSession is Windows-only in first implementation");
    return;
#else
    auto options = baseOptions("step7-runtime-error-test");
    options.controllerTickCommandName = "controller.invalid";

    celestia::runtime::process::RuntimeSession session(options);
    const auto result = session.run();

    CAPTURE(result.log);
    CHECK_FALSE(result.success);
    CHECK(result.controllerReady);
    CHECK(result.modelReady);
    CHECK(result.viewReady);
    CHECK(contains(result.log, "controller runtime.error"));
    CHECK(contains(result.log, "payload="));
    CHECK(contains(result.log, "all hosts stopped"));
#endif
}

TEST_CASE("RuntimeSession tracks heartbeat replies from live hosts")
{
#ifndef _WIN32
    MESSAGE("Step7 live RuntimeSession is Windows-only in first implementation");
    return;
#else
    auto options = baseOptions("step7-heartbeat-test");
    options.heartbeatEveryTicks = 2;

    celestia::runtime::process::RuntimeSession session(options);
    const auto result = session.run();

    CAPTURE(result.log);
    CHECK(result.success);
    CHECK(result.heartbeatCount >= 9);
    CHECK(contains(result.log, "heartbeat count="));
#endif
}

TEST_CASE("RuntimeSession terminates hosts when shutdown timeout expires")
{
#ifndef _WIN32
    MESSAGE("Step7 live RuntimeSession is Windows-only in first implementation");
    return;
#else
    auto options = baseOptions("step7-shutdown-timeout-test");
    options.shutdownTimeoutMilliseconds = 0;

    celestia::runtime::process::RuntimeSession session(options);
    const auto result = session.run();

    CAPTURE(result.log);
    CHECK_FALSE(result.success);
    CHECK(result.terminatedHost);
    CHECK(contains(result.log, "shutdown wait timeout"));
#endif
}

TEST_SUITE_END();
