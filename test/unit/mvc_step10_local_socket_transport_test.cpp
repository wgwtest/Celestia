#include <doctest.h>

#include <chrono>
#include <filesystem>
#include <string>

#include <celruntime/process/hostprocess.h>
#include <celruntime/protocol/lifecycle.h>
#include <celruntime/runtimeconfig.h>
#include <celruntime/transport/transportfactory.h>

namespace
{

std::filesystem::path
buildRoot()
{
    return std::filesystem::current_path().parent_path().parent_path();
}

celestia::runtime::protocol::RuntimeEnvelope
lifecycle(std::string name, std::uint64_t sequence)
{
    auto envelope = celestia::runtime::protocol::lifecycle(
        celestia::runtime::protocol::RuntimeRole::Launcher,
        celestia::runtime::protocol::RuntimeRole::Model,
        std::move(name));
    envelope.sessionId = "step10-local-socket-test";
    envelope.sequenceId = sequence;
    return envelope;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step10 local socket transport");

TEST_CASE("local-socket transport exchanges lifecycle frames with a live host")
{
#ifndef _WIN32
    MESSAGE("Step10 local-socket first implementation is Windows named-pipe based");
    return;
#else
    celestia::runtime::process::HostProcessOptions options;
    options.executable = buildRoot() / "src" / "celruntime" / "celestia-model-host.exe";
    options.workingDirectory = buildRoot() / "src" / "celruntime";
    options.arguments = {
        "--protocol-version=1",
        "--serve",
        "--view=" + std::string(celestia::runtime::RuntimeConfig::Debug2DViewId),
        "--session=step10-local-socket-test",
    };

    std::string error;
    auto transport = celestia::runtime::transport::createRuntimeTransport(
        "local-socket", std::move(options), &error);
    REQUIRE_MESSAGE(transport != nullptr, error);
    REQUIRE_MESSAGE(transport->open(&error), error);

    REQUIRE(transport->send(lifecycle(celestia::runtime::protocol::RuntimeHello, 1)));
    const auto ready = transport->receive(std::chrono::milliseconds(3000));
    REQUIRE(ready.has_value());
    CHECK(ready->name == celestia::runtime::protocol::RuntimeReady);

    REQUIRE(transport->send(lifecycle(celestia::runtime::protocol::RuntimeShutdown, 2)));
    const auto stopped = transport->receive(std::chrono::milliseconds(3000));
    REQUIRE(stopped.has_value());
    CHECK(stopped->name == celestia::runtime::protocol::RuntimeStopped);

    const auto exitCode = transport->wait(std::chrono::milliseconds(3000));
    REQUIRE(exitCode.has_value());
    CHECK(*exitCode == 0);
    CHECK(transport->stats().sentMessages == 2);
    CHECK(transport->stats().receivedMessages == 2);
#endif
}

TEST_SUITE_END();
