#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <celruntime/controller/controllerservice.h>
#include <celruntime/model/modelservice.h>
#include <celruntime/protocol/envelope.h>
#include <celruntime/protocol/lifecycle.h>
#include <celruntime/protocol/sceneprotocol.h>
#include <celruntime/protocol/viewinput.h>
#include <celruntime/view3d/view3dhost.h>

namespace
{

using celestia::runtime::protocol::RuntimeEnvelope;
using celestia::runtime::protocol::RuntimeMessageKind;
using celestia::runtime::protocol::RuntimeRole;

bool
contains(std::string_view text, std::string_view token)
{
    return text.find(token) != std::string_view::npos;
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

celestia::runtime::protocol::ViewInputEvent
spaceKeyDown()
{
    celestia::runtime::protocol::ViewInputEvent input;
    input.sessionId = "step16-controller-interaction";
    input.sequence = 16;
    input.device = "keyboard";
    input.action = "KeyDown";
    input.key = "Space";
    return input;
}

RuntimeEnvelope
runtimeStart(RuntimeRole target)
{
    auto envelope = celestia::runtime::protocol::lifecycle(
        RuntimeRole::Launcher,
        target,
        celestia::runtime::protocol::RuntimeStart);
    envelope.sessionId = "step16-controller-interaction";
    return envelope;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step16 controller interaction");

TEST_CASE("Step16 view.input Space key toggles time pause through Controller Model and View3D")
{
    celestia::runtime::controller::ControllerService controller("step16-controller-interaction");
    celestia::runtime::model::ModelService model("step16-controller-interaction");
    celestia::runtime::view3d::View3DHost view("step16-controller-interaction");

    controller.handle(runtimeStart(RuntimeRole::Controller));
    model.handle(runtimeStart(RuntimeRole::Model));
    view.handle(runtimeStart(RuntimeRole::View));

    const auto controllerCommands = controller.handle(celestia::runtime::protocol::viewInputEnvelope(
        spaceKeyDown(),
        RuntimeRole::View,
        RuntimeRole::Controller));

    REQUIRE(controllerCommands.size() == 1);
    CHECK(controllerCommands.front().kind == RuntimeMessageKind::Command);
    CHECK(controllerCommands.front().targetRole == RuntimeRole::Model);
    CHECK(controllerCommands.front().name == "model.setPaused");
    CHECK(contains(controllerCommands.front().payload, "paused=true"));
    CHECK(contains(controllerCommands.front().payload, "view=celestia.view3d.opengl"));
    CHECK(contains(controllerCommands.front().payload, "command=time.pause"));

    const auto modelFrame = model.handle(controllerCommands.front());
    REQUIRE(modelFrame.kind == RuntimeMessageKind::ViewFrame);
    REQUIRE(modelFrame.name == celestia::runtime::protocol::SceneFrameMessageName);

    const auto scene = celestia::runtime::protocol::deserializeSceneFrame(modelFrame.payload);
    REQUIRE(scene.has_value());
    CHECK(scene->time.paused);

    const auto rendered = view.handle(modelFrame);
    REQUIRE(rendered.size() == 1);
    CHECK(rendered.front().name == "view.frameRendered");
    CHECK(contains(rendered.front().payload, "frameCount=1"));
}

TEST_CASE("Step16 RuntimeSession view.input route accepts typed model commands")
{
    const auto source = readSourceFile("src/celruntime/process/runtimesession.cpp");

    CHECK(contains(source, "protocol::ViewInputMessageName"));
    CHECK(contains(source, "controllerMessage->kind != RuntimeMessageKind::Command"));
    CHECK_FALSE(contains(source, "controllerMessage->name != \"model.setViewInput\""));
    CHECK(contains(source, "view.input routed count="));
}

TEST_SUITE_END();
