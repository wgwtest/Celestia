#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

#include <celruntime/assembly/runtimeassemblyconfig.h>
#include <celruntime/assembly/runtimeassemblyrunner.h>
#include <celruntime/model/modelsnapshot.h>
#include <celruntime/model/realmodelbackend.h>
#include <celruntime/process/runtimehostloop.h>
#include <celruntime/protocol/lifecycle.h>
#include <celruntime/runtimeconfig.h>
#include <celruntime/transport/framedmessage.h>

namespace
{

std::filesystem::path
buildRoot()
{
    return std::filesystem::current_path().parent_path().parent_path();
}

std::filesystem::path
sourceRoot()
{
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
}

std::filesystem::path
contentRoot()
{
    return buildRoot() / "run-full";
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

bool
hasRealCelestiaSelection(const celestia::runtime::ViewFrame& frame)
{
    for (const auto& selection : frame.selections)
    {
        if ((selection.type == "body" || selection.type == "star") &&
            !selection.id.empty() &&
            selection.id.find("Synthetic") == std::string::npos)
        {
            return true;
        }
    }

    return false;
}

celestia::runtime::protocol::RuntimeEnvelope
envelope(celestia::runtime::protocol::RuntimeMessageKind kind,
         std::string name,
         std::uint64_t sequence)
{
    celestia::runtime::protocol::RuntimeEnvelope message;
    message.sessionId = "step13-model-host-loop";
    message.sequenceId = sequence;
    message.sourceRole = celestia::runtime::protocol::RuntimeRole::Launcher;
    message.targetRole = celestia::runtime::protocol::RuntimeRole::Model;
    message.kind = kind;
    message.name = std::move(name);
    return message;
}

std::vector<celestia::runtime::protocol::RuntimeEnvelope>
decodeFrames(std::string_view text)
{
    celestia::runtime::transport::FramedMessageReader reader;
    reader.append(text);
    reader.close();

    std::vector<celestia::runtime::protocol::RuntimeEnvelope> messages;
    for (;;)
    {
        auto received = reader.receive();
        if (received.status == celestia::runtime::transport::ReceiveStatus::Closed)
            return messages;

        REQUIRE(received.status == celestia::runtime::transport::ReceiveStatus::Message);
        REQUIRE(received.message.has_value());
        messages.push_back(std::move(*received.message));
    }
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step13 real model backend");

TEST_CASE("Step13 RuntimeConfig carries a configured data root into assembly resources")
{
    REQUIRE(std::filesystem::exists(contentRoot() / "celestia.cfg"));
    REQUIRE(std::filesystem::exists(contentRoot() / "data" / "stars.dat"));

    celestia::runtime::RuntimeConfig runtimeConfig;
    const auto dataRootArgument = "--data-root=" + contentRoot().string();
    REQUIRE(celestia::runtime::applyRuntimeConfigArgument(runtimeConfig, dataRootArgument));
    CHECK(runtimeConfig.dataRoot() == contentRoot().string());

    auto assembly = celestia::runtime::assembly::RuntimeAssemblyConfig::fromRuntimeConfig(
        runtimeConfig,
        buildRoot() / "src" / "celruntime",
        {},
        "step13-runtime-config");

    CHECK(assembly.resources.contentRoot == contentRoot());
}

TEST_CASE("Step13 RealModelBackend loads real Celestia data headlessly")
{
    REQUIRE(std::filesystem::exists(contentRoot() / "celestia.cfg"));
    REQUIRE(std::filesystem::exists(contentRoot() / "data" / "solarsys.ssc"));

    auto backend = celestia::runtime::model::createRealModelBackend();
    REQUIRE(backend != nullptr);

    celestia::runtime::model::RuntimeDataPaths paths;
    paths.dataRoot = contentRoot().string();
    REQUIRE(backend->load(paths));

    backend->setTime(2451545.0);
    const auto frame = backend->snapshot();

    CHECK(frame.time == doctest::Approx(2451545.0));
    CHECK(contains(frame.summary, "real Celestia model"));
    REQUIRE_FALSE(frame.selections.empty());
    CHECK(hasRealCelestiaSelection(frame));
}

TEST_CASE("Step13 model host loop serves real backend frames when configured with data root")
{
    celestia::runtime::model::RuntimeDataPaths paths;
    paths.dataRoot = contentRoot().string();

    std::stringstream input;
    input << celestia::runtime::transport::encodeFrame(
        envelope(celestia::runtime::protocol::RuntimeMessageKind::Lifecycle,
                 celestia::runtime::protocol::RuntimeHello,
                 1));
    input << celestia::runtime::transport::encodeFrame(
        envelope(celestia::runtime::protocol::RuntimeMessageKind::Lifecycle,
                 celestia::runtime::protocol::RuntimeStart,
                 2));
    input << celestia::runtime::transport::encodeFrame(
        envelope(celestia::runtime::protocol::RuntimeMessageKind::Command,
                 "model.requestSnapshot",
                 3));
    input << celestia::runtime::transport::encodeFrame(
        envelope(celestia::runtime::protocol::RuntimeMessageKind::Lifecycle,
                 celestia::runtime::protocol::RuntimeShutdown,
                 4));

    std::stringstream output;
    std::stringstream error;

    const auto exitCode = celestia::runtime::process::runRuntimeModelHostLoop(
        "step13-model-host-loop",
        celestia::runtime::model::createRealModelBackend(),
        paths,
        input,
        output,
        error);

    CAPTURE(error.str());
    CHECK(exitCode == 0);

    const auto messages = decodeFrames(output.str());
    REQUIRE(messages.size() == 4);
    CHECK(messages[0].name == celestia::runtime::protocol::RuntimeReady);
    CHECK(messages[1].name == "runtime.started");
    REQUIRE(messages[2].name == "view.frame");

    const auto frame = celestia::runtime::model::deserializeViewFrame(messages[2].payload);
    REQUIRE(frame.has_value());
    CHECK(contains(frame->summary, "real Celestia model"));
    CHECK(hasRealCelestiaSelection(*frame));
    CHECK(messages[3].name == celestia::runtime::protocol::RuntimeStopped);
}

TEST_CASE("Step13 runtime session passes data root to the model host only")
{
    const auto sessionSource = readSourceFile("src/celruntime/process/runtimesession.cpp");
    const auto hostSource = readSourceFile("src/celruntime/process/runtimehostcommon.cpp");

    CHECK(contains(sessionSource, "--data-root="));
    CHECK(contains(sessionSource, "host.role == RuntimeRole::Model"));
    CHECK(contains(hostSource, "--data-root="));
}

TEST_CASE("Step13 runtime assembly starts the model host process with resolved real data root")
{
#ifndef _WIN32
    MESSAGE("Step13 live process data-root assembly check is Windows-only in this build");
    return;
#else
    celestia::runtime::RuntimeConfig runtimeConfig;
    runtimeConfig.setRuntimeMode(celestia::runtime::RuntimeMode::MultiProcess);
    runtimeConfig.setSelectedViewId(std::string(celestia::runtime::RuntimeConfig::Debug2DViewId));
    runtimeConfig.setHostTransport("stdio-pipe");
    runtimeConfig.setDurationMilliseconds(120);

    auto assembly = celestia::runtime::assembly::RuntimeAssemblyConfig::fromRuntimeConfig(
        runtimeConfig,
        buildRoot() / "src" / "celruntime",
        buildRoot(),
        "step13-real-model-process");
    assembly.session.tickMilliseconds = 20;
    assembly.session.readyTimeoutMilliseconds = 8000;
    assembly.session.shutdownTimeoutMilliseconds = 5000;

    celestia::runtime::assembly::RuntimeAssemblyRunner runner(std::move(assembly));
    const auto result = runner.run();

    CAPTURE(result.log);
    CHECK(result.success);
    CHECK(contains(result.log, "model dataRoot="));
    CHECK(contains(result.log, "run-full"));
#endif
}

TEST_CASE("Step13 real backend source stays headless and renderer-free")
{
    const auto header = readSourceFile("src/celruntime/model/realmodelbackend.h");
    const auto source = readSourceFile("src/celruntime/model/realmodelbackend.cpp");

    CHECK_FALSE(contains(header, "SDL"));
    CHECK_FALSE(contains(source, "SDL"));
    CHECK_FALSE(contains(header, "OpenGL"));
    CHECK_FALSE(contains(source, "OpenGL"));
    CHECK_FALSE(contains(header, "Renderer"));
    CHECK_FALSE(contains(source, "Renderer"));
}

TEST_SUITE_END();
