#include <doctest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <celruntime/process/hostprocess.h>
#include <celruntime/process/view3dhostloop.h>
#include <celruntime/protocol/envelope.h>
#include <celruntime/protocol/lifecycle.h>
#include <celruntime/protocol/sceneprotocol.h>
#include <celruntime/protocol/viewinput.h>
#include <celruntime/transport/framedmessage.h>
#include <celruntime/view3d/view3dhost.h>

namespace
{

using celestia::runtime::protocol::RuntimeEnvelope;
using celestia::runtime::protocol::RuntimeMessageKind;
using celestia::runtime::protocol::RuntimeRole;

std::vector<celestia::runtime::protocol::ViewInputEvent> g_pendingInputs;

std::vector<celestia::runtime::protocol::ViewInputEvent>
drainTestInputs()
{
    auto events = std::move(g_pendingInputs);
    g_pendingInputs.clear();
    return events;
}

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

RuntimeEnvelope
lifecycle(std::string name)
{
    auto envelope = celestia::runtime::protocol::lifecycle(RuntimeRole::Launcher,
                                                          RuntimeRole::View,
                                                          std::move(name));
    envelope.sessionId = "step8-view3d-host-test";
    return envelope;
}

celestia::runtime::protocol::SceneFrame
minimalSceneFrame()
{
    celestia::runtime::protocol::SceneFrame frame;
    frame.sessionId = "step8-view3d-host-test";
    frame.sequence = 11;
    frame.simulationTime = 42.0;
    frame.camera.fov = 45.0;
    frame.renderSettings.showStars = true;

    celestia::runtime::protocol::BodyRenderState body;
    body.bodyId = "Earth";
    body.name = "Earth";
    body.visible = true;
    body.radius = 6371.0;
    frame.bodies.push_back(std::move(body));

    celestia::runtime::protocol::StarRenderState star;
    star.starId = "Sol";
    frame.stars.push_back(std::move(star));
    return frame;
}

RuntimeEnvelope
sceneFrameEnvelope()
{
    return celestia::runtime::protocol::sceneFrameEnvelope(minimalSceneFrame(),
                                                          RuntimeRole::Model,
                                                          RuntimeRole::View);
}

std::vector<RuntimeEnvelope>
decodeFrames(std::string_view output)
{
    celestia::runtime::transport::FramedMessageReader reader;
    reader.append(output);

    std::vector<RuntimeEnvelope> messages;
    for (;;)
    {
        auto received = reader.receive();
        if (received.status == celestia::runtime::transport::ReceiveStatus::WouldBlock ||
            received.status == celestia::runtime::transport::ReceiveStatus::Closed)
        {
            return messages;
        }
        REQUIRE(received.status == celestia::runtime::transport::ReceiveStatus::Message);
        REQUIRE(received.message.has_value());
        messages.push_back(std::move(*received.message));
    }
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step8 view3d host");

TEST_CASE("View3D host declares optional SDL OpenGL render loop without leaking SDL types")
{
    const auto cmake = readSourceFile("src/celruntime/CMakeLists.txt");
    const auto loopHeader = readSourceFile("src/celruntime/view3d/view3dloop.h");

    CHECK(contains(cmake, "view3d/view3dloop.cpp"));
    CHECK(contains(cmake, "CELESTIA_RUNTIME_VIEW3D_SDL"));
    CHECK(contains(loopHeader, "class View3DLoop"));
    CHECK_FALSE(contains(loopHeader, "SDL_Window"));
    CHECK_FALSE(contains(loopHeader, "SDL_GLContext"));
}

TEST_CASE("View3D host service consumes scene frames and reports rendered frames")
{
    celestia::runtime::view3d::View3DHost service("step8-view3d-host-test");
    CHECK_FALSE(service.isRunning());

    const auto started = service.handle(lifecycle(celestia::runtime::protocol::RuntimeStart));
    REQUIRE(started.size() == 1);
    CHECK(service.isRunning());
    CHECK(started.front().kind == RuntimeMessageKind::Event);
    CHECK(started.front().name == "view.ready3d");

    const auto rendered = service.handle(sceneFrameEnvelope());
    REQUIRE(rendered.size() == 1);
    CHECK(service.frameCount() == 1);
    CHECK(service.lastSequence() == 11);
    CHECK(service.lastSimulationTime() == doctest::Approx(42.0));
    CHECK(rendered.front().name == "view.frameRendered");
    CHECK(rendered.front().payload.find("frameCount=1") != std::string::npos);
}

TEST_CASE("View3D host loop stamps drained input events with the current runtime session")
{
    g_pendingInputs.clear();
    celestia::runtime::protocol::ViewInputEvent input;
    input.sequence = 7;
    input.device = "mouse";
    input.action = "MouseMove";
    input.pointer = { 120.0, 80.0 };
    g_pendingInputs.push_back(input);

    celestia::runtime::process::View3DHostCallbacks callbacks;
    callbacks.drainInput = drainTestInputs;
    celestia::runtime::process::setRuntimeView3DHostCallbacks(callbacks);

    std::stringstream inputStream;
    std::stringstream outputStream;
    std::stringstream errorStream;

    auto hello = lifecycle(celestia::runtime::protocol::RuntimeHello);
    hello.sessionId = "step8-view3d-loop-session";
    auto start = lifecycle(celestia::runtime::protocol::RuntimeStart);
    start.sessionId = "step8-view3d-loop-session";
    auto frame = sceneFrameEnvelope();
    frame.sessionId = "step8-view3d-loop-session";
    auto shutdown = lifecycle(celestia::runtime::protocol::RuntimeShutdown);
    shutdown.sessionId = "step8-view3d-loop-session";

    inputStream << celestia::runtime::transport::encodeFrame(hello)
                << celestia::runtime::transport::encodeFrame(start)
                << celestia::runtime::transport::encodeFrame(frame)
                << celestia::runtime::transport::encodeFrame(shutdown);

    const auto exitCode = celestia::runtime::process::runRuntimeView3DHostLoop(
        "step8-view3d-loop-session",
        inputStream,
        outputStream,
        errorStream);
    celestia::runtime::process::setRuntimeView3DHostCallbacks({});

    CAPTURE(errorStream.str());
    CHECK(exitCode == 0);

    const auto messages = decodeFrames(outputStream.str());
    bool foundInput = false;
    for (const auto& message : messages)
    {
        if (message.name != celestia::runtime::protocol::ViewInputMessageName)
            continue;

        foundInput = true;
        CHECK(message.sessionId == "step8-view3d-loop-session");
        CHECK(message.sourceRole == RuntimeRole::View);
        CHECK(message.targetRole == RuntimeRole::Controller);

        const auto parsed = celestia::runtime::protocol::deserializeViewInputEvent(message.payload);
        REQUIRE(parsed.has_value());
        CHECK(parsed->sessionId == "step8-view3d-loop-session");
        CHECK(parsed->action == "MouseMove");
        CHECK(parsed->pointer[0] == doctest::Approx(120.0));
    }
    CHECK(foundInput);
}

TEST_CASE("celestia-view3d-host exchanges lifecycle and scene frames over live stdio pipes")
{
#ifndef _WIN32
    MESSAGE("Step8 live View3D HostProcess is Windows-only in first implementation");
    return;
#else
    const auto executable = hostPath("celestia-view3d-host");
    CAPTURE(executable.string());
    REQUIRE(std::filesystem::exists(executable));

    celestia::runtime::process::HostProcessOptions options;
    options.executable = executable;
    options.workingDirectory = buildRoot();
    options.arguments = {
        "--stdio",
        "--protocol-version=1",
        "--view=celestia.view3d.opengl",
        "--serve",
        "--session=step8-view3d-host-test",
    };

    celestia::runtime::process::HostProcess host(options);
    REQUIRE(host.start());
    CHECK(host.running());

    REQUIRE(host.send(lifecycle(celestia::runtime::protocol::RuntimeHello)));
    const auto ready = host.receive(std::chrono::milliseconds(1000));
    REQUIRE(ready.has_value());
    CHECK(ready->name == celestia::runtime::protocol::RuntimeReady);
    CHECK(ready->sourceRole == RuntimeRole::View);

    REQUIRE(host.send(lifecycle(celestia::runtime::protocol::RuntimeStart)));
    const auto ready3d = host.receive(std::chrono::milliseconds(1000));
    REQUIRE(ready3d.has_value());
    CHECK(ready3d->name == "view.ready3d");
    CHECK(ready3d->sourceRole == RuntimeRole::View);

    REQUIRE(host.send(sceneFrameEnvelope()));
    const auto rendered = host.receive(std::chrono::milliseconds(1000));
    REQUIRE(rendered.has_value());
    CHECK(rendered->name == "view.frameRendered");
    CHECK(rendered->payload.find("lastSequence=11") != std::string::npos);

    REQUIRE(host.send(lifecycle(celestia::runtime::protocol::RuntimeShutdown)));
    const auto stopped = host.receive(std::chrono::milliseconds(1000));
    REQUIRE(stopped.has_value());
    CHECK(stopped->name == celestia::runtime::protocol::RuntimeStopped);

    const auto exitCode = host.wait(std::chrono::milliseconds(1000));
    REQUIRE(exitCode.has_value());
    CHECK(*exitCode == 0);
    CHECK_FALSE(host.running());
#endif
}

TEST_SUITE_END();
