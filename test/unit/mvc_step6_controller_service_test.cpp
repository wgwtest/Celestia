#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <celruntime/controller/controllerservice.h>
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
viewEvent(std::string name, std::string payload = {})
{
    RuntimeEnvelope envelope;
    envelope.sessionId = "controller-service-test";
    envelope.sourceRole = RuntimeRole::View;
    envelope.targetRole = RuntimeRole::Controller;
    envelope.kind = RuntimeMessageKind::Event;
    envelope.name = std::move(name);
    envelope.payload = std::move(payload);
    return envelope;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step6 controller service");

TEST_CASE("ControllerService maps input events to model commands")
{
    celestia::runtime::controller::ControllerService service("controller-service-test");

    auto start = celestia::runtime::protocol::lifecycle(RuntimeRole::Launcher,
                                                       RuntimeRole::Controller,
                                                       "runtime.start");
    start.sessionId = "controller-service-test";
    const auto started = service.handle(start);
    REQUIRE(started.size() == 1);
    CHECK(service.isRunning());
    CHECK(started.front().kind == RuntimeMessageKind::Lifecycle);
    CHECK(started.front().name == "controller.running");

    const auto paused = service.handle(viewEvent("input.key", "key=Space"));
    REQUIRE(paused.size() == 1);
    CHECK(paused.front().sourceRole == RuntimeRole::Controller);
    CHECK(paused.front().targetRole == RuntimeRole::Model);
    CHECK(paused.front().kind == RuntimeMessageKind::Command);
    CHECK(paused.front().name == "model.setPaused");
    CHECK(paused.front().payload == "paused=true");

    const auto resumed = service.handle(viewEvent("input.key", "key=Space"));
    REQUIRE(resumed.size() == 1);
    CHECK(resumed.front().name == "model.setPaused");
    CHECK(resumed.front().payload == "paused=false");

    RuntimeEnvelope tick;
    tick.sessionId = "controller-service-test";
    tick.sourceRole = RuntimeRole::Launcher;
    tick.targetRole = RuntimeRole::Controller;
    tick.kind = RuntimeMessageKind::Command;
    tick.name = "controller.tick";
    const auto snapshot = service.handle(tick);
    REQUIRE(snapshot.size() == 1);
    CHECK(snapshot.front().targetRole == RuntimeRole::Model);
    CHECK(snapshot.front().kind == RuntimeMessageKind::Command);
    CHECK(snapshot.front().name == "model.requestSnapshot");
}

TEST_CASE("ControllerService handles close and unknown input explicitly")
{
    celestia::runtime::controller::ControllerService service("controller-service-test");

    const auto close = service.handle(viewEvent("input.closeRequested"));
    REQUIRE(close.size() == 1);
    CHECK(close.front().kind == RuntimeMessageKind::Lifecycle);
    CHECK(close.front().name == "runtime.shutdown");
    CHECK(close.front().targetRole == RuntimeRole::Broadcast);

    const auto unknown = service.handle(viewEvent("input.wheel", "delta=1"));
    REQUIRE(unknown.size() == 1);
    CHECK(unknown.front().kind == RuntimeMessageKind::Error);
    CHECK(unknown.front().name == "runtime.error");
}

TEST_CASE("ControllerService stays independent from Model implementation")
{
    const auto controllerHeader = readSourceFile("src/celruntime/controller/controllerservice.h");
    const auto controllerSource = readSourceFile("src/celruntime/controller/controllerservice.cpp");

    CHECK_FALSE(contains(controllerHeader, "modelservice.h"));
    CHECK_FALSE(contains(controllerSource, "modelservice.h"));
    CHECK_FALSE(contains(controllerHeader, "ModelService"));
    CHECK_FALSE(contains(controllerSource, "ModelService"));
}

TEST_SUITE_END();
