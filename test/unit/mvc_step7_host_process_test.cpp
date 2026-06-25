#include <doctest.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <celruntime/process/hostprocess.h>
#include <celruntime/protocol/envelope.h>
#include <celruntime/protocol/lifecycle.h>

namespace
{

std::filesystem::path
buildRoot()
{
    return std::filesystem::current_path().parent_path().parent_path();
}

std::filesystem::path
hostPath(std::string_view target)
{
#ifdef _WIN32
    return buildRoot() / "src" / "celruntime" / (std::string(target) + ".exe");
#else
    return buildRoot() / "src" / "celruntime" / std::string(target);
#endif
}

celestia::runtime::protocol::RuntimeEnvelope
lifecycle(std::string name)
{
    auto envelope = celestia::runtime::protocol::lifecycle(
        celestia::runtime::protocol::RuntimeRole::Launcher,
        celestia::runtime::protocol::RuntimeRole::Model,
        std::move(name));
    envelope.sessionId = "step7-host-process-test";
    return envelope;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step7 host process");

TEST_CASE("HostProcess exchanges lifecycle frames over live stdio pipes")
{
#ifndef _WIN32
    MESSAGE("Step7 live HostProcess is Windows-only in first implementation");
    return;
#else
    const auto executable = hostPath("celestia-model-host");
    CAPTURE(executable.string());
    REQUIRE(std::filesystem::exists(executable));

    celestia::runtime::process::HostProcessOptions options;
    options.executable = executable;
    options.workingDirectory = buildRoot();
    options.arguments = {
        "--stdio",
        "--protocol-version=1",
        "--view=celestia.view2d.debug",
        "--serve",
        "--session=step7-host-process-test",
    };

    celestia::runtime::process::HostProcess host(options);
    REQUIRE(host.start());
    CHECK(host.running());

    REQUIRE(host.send(lifecycle(celestia::runtime::protocol::RuntimeHello)));

    const auto ready = host.receive(std::chrono::milliseconds(1000));
    REQUIRE(ready.has_value());
    CHECK(ready->name == celestia::runtime::protocol::RuntimeReady);
    CHECK(ready->sourceRole == celestia::runtime::protocol::RuntimeRole::Model);
    CHECK(ready->targetRole == celestia::runtime::protocol::RuntimeRole::Launcher);

    REQUIRE(host.send(lifecycle(celestia::runtime::protocol::RuntimeShutdown)));

    const auto stopped = host.receive(std::chrono::milliseconds(1000));
    REQUIRE(stopped.has_value());
    CHECK(stopped->name == celestia::runtime::protocol::RuntimeStopped);
    CHECK(stopped->sourceRole == celestia::runtime::protocol::RuntimeRole::Model);
    CHECK(stopped->targetRole == celestia::runtime::protocol::RuntimeRole::Launcher);

    const auto exitCode = host.wait(std::chrono::milliseconds(1000));
    REQUIRE(exitCode.has_value());
    CHECK(*exitCode == 0);
    CHECK_FALSE(host.running());
#endif
}

TEST_SUITE_END();
