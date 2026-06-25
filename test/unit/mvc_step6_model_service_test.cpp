#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <celruntime/model/modelservice.h>
#include <celruntime/model/modelsnapshot.h>
#include <celruntime/protocol/envelope.h>

namespace
{

using celestia::runtime::protocol::RuntimeEnvelope;
using celestia::runtime::protocol::RuntimeMessageKind;
using celestia::runtime::protocol::RuntimeRole;

std::filesystem::path
sourceRoot()
{
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
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

RuntimeEnvelope
modelCommand(std::string name, std::string payload = {})
{
    RuntimeEnvelope envelope;
    envelope.sessionId = "model-service-test";
    envelope.sourceRole = RuntimeRole::Controller;
    envelope.targetRole = RuntimeRole::Model;
    envelope.kind = RuntimeMessageKind::Command;
    envelope.name = std::move(name);
    envelope.payload = std::move(payload);
    return envelope;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step6 model service");

TEST_CASE("ModelService runs a long-lived synthetic simulation")
{
    celestia::runtime::model::ModelService service("model-service-test");
    CHECK_FALSE(service.isRunning());

    auto start = celestia::runtime::protocol::lifecycle(RuntimeRole::Launcher,
                                                       RuntimeRole::Model,
                                                       "runtime.start");
    start.sessionId = "model-service-test";
    const auto started = service.handle(start);
    CHECK(service.isRunning());
    CHECK(started.kind == RuntimeMessageKind::Lifecycle);
    CHECK(started.name == "runtime.started");

    const auto stepped = service.handle(modelCommand("model.step", "dt=2.5"));
    REQUIRE(stepped.kind == RuntimeMessageKind::ViewFrame);
    REQUIRE(stepped.name == "view.frame");
    const auto firstFrame = celestia::runtime::model::deserializeViewFrame(stepped.payload);
    REQUIRE(firstFrame.has_value());
    CHECK(firstFrame->time == doctest::Approx(2.5));
    CHECK(firstFrame->frameId == 1);
    CHECK(firstFrame->summary.find("synthetic") != std::string::npos);

    service.handle(modelCommand("model.setPaused", "paused=true"));
    const auto pausedStep = service.handle(modelCommand("model.step", "dt=3.0"));
    const auto pausedFrame = celestia::runtime::model::deserializeViewFrame(pausedStep.payload);
    REQUIRE(pausedFrame.has_value());
    CHECK(pausedFrame->time == doctest::Approx(2.5));

    const auto snapshot = service.handle(modelCommand("model.requestSnapshot"));
    REQUIRE(snapshot.kind == RuntimeMessageKind::ViewFrame);
    REQUIRE(snapshot.name == "view.frame");
    const auto snapshotFrame = celestia::runtime::model::deserializeViewFrame(snapshot.payload);
    REQUIRE(snapshotFrame.has_value());
    CHECK(snapshotFrame->time == doctest::Approx(2.5));
}

TEST_CASE("ModelService keeps process boundaries pointer-free")
{
    const auto serviceHeader = readSourceFile("src/celruntime/model/modelservice.h");
    const auto snapshotHeader = readSourceFile("src/celruntime/model/modelsnapshot.h");

    CHECK(contains(snapshotHeader, "class SimulationBackend"));
    CHECK(contains(snapshotHeader, "ViewFrame snapshot() const"));

    CHECK_FALSE(contains(serviceHeader, "Simulation*"));
    CHECK_FALSE(contains(serviceHeader, "Universe*"));
    CHECK_FALSE(contains(serviceHeader, "Renderer*"));
    CHECK_FALSE(contains(snapshotHeader, "Simulation*"));
    CHECK_FALSE(contains(snapshotHeader, "Universe*"));
    CHECK_FALSE(contains(snapshotHeader, "Renderer*"));
}

TEST_SUITE_END();
