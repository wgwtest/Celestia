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
keyDown(std::string key, std::string modifiers = {})
{
    celestia::runtime::protocol::ViewInputEvent input;
    input.sessionId = "step16-controller-interaction";
    input.sequence = 16;
    input.device = "keyboard";
    input.action = "KeyDown";
    input.key = std::move(key);
    input.modifiers = std::move(modifiers);
    return input;
}

celestia::runtime::protocol::ViewInputEvent
mouseWheel(double wheelY)
{
    celestia::runtime::protocol::ViewInputEvent input;
    input.sessionId = "step16-controller-interaction";
    input.sequence = 17;
    input.device = "mouse";
    input.action = "MouseWheel";
    input.pointer = { 320.0, 240.0 };
    input.wheel = { 0.0, wheelY };
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
        keyDown("Space"),
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

TEST_CASE("Step16 view.input L key changes time scale through Controller Model and View3D")
{
    celestia::runtime::controller::ControllerService controller("step16-controller-interaction");
    celestia::runtime::model::ModelService model("step16-controller-interaction");
    celestia::runtime::view3d::View3DHost view("step16-controller-interaction");

    controller.handle(runtimeStart(RuntimeRole::Controller));
    model.handle(runtimeStart(RuntimeRole::Model));
    view.handle(runtimeStart(RuntimeRole::View));

    const auto controllerCommands = controller.handle(celestia::runtime::protocol::viewInputEnvelope(
        keyDown("L"),
        RuntimeRole::View,
        RuntimeRole::Controller));

    REQUIRE(controllerCommands.size() == 1);
    CHECK(controllerCommands.front().kind == RuntimeMessageKind::Command);
    CHECK(controllerCommands.front().targetRole == RuntimeRole::Model);
    CHECK(controllerCommands.front().name == "model.setTimeScale");
    CHECK(contains(controllerCommands.front().payload, "timeScale=2"));
    CHECK(contains(controllerCommands.front().payload, "view=celestia.view3d.opengl"));
    CHECK(contains(controllerCommands.front().payload, "command=time.setScale"));

    const auto modelFrame = model.handle(controllerCommands.front());
    REQUIRE(modelFrame.kind == RuntimeMessageKind::ViewFrame);
    REQUIRE(modelFrame.name == celestia::runtime::protocol::SceneFrameMessageName);

    const auto scene = celestia::runtime::protocol::deserializeSceneFrame(modelFrame.payload);
    REQUIRE(scene.has_value());
    CHECK(scene->time.timeScale == doctest::Approx(2.0));

    const auto rendered = view.handle(modelFrame);
    REQUIRE(rendered.size() == 1);
    CHECK(rendered.front().name == "view.frameRendered");
    CHECK(contains(rendered.front().payload, "frameCount=1"));
}

TEST_CASE("Step16 mouse wheel zoom changes scene camera fov through Controller Model and View3D")
{
    celestia::runtime::controller::ControllerService controller("step16-controller-interaction");
    celestia::runtime::model::ModelService model("step16-controller-interaction");
    celestia::runtime::view3d::View3DHost view("step16-controller-interaction");

    controller.handle(runtimeStart(RuntimeRole::Controller));
    model.handle(runtimeStart(RuntimeRole::Model));
    view.handle(runtimeStart(RuntimeRole::View));

    const auto controllerCommands = controller.handle(celestia::runtime::protocol::viewInputEnvelope(
        mouseWheel(1.0),
        RuntimeRole::View,
        RuntimeRole::Controller));

    REQUIRE(controllerCommands.size() == 1);
    CHECK(controllerCommands.front().kind == RuntimeMessageKind::Command);
    CHECK(controllerCommands.front().targetRole == RuntimeRole::Model);
    CHECK(controllerCommands.front().name == "model.setCameraFov");
    CHECK(contains(controllerCommands.front().payload, "fov=40"));
    CHECK(contains(controllerCommands.front().payload, "view=celestia.view3d.opengl"));
    CHECK(contains(controllerCommands.front().payload, "command=camera.zoom"));

    const auto modelFrame = model.handle(controllerCommands.front());
    REQUIRE(modelFrame.kind == RuntimeMessageKind::ViewFrame);
    REQUIRE(modelFrame.name == celestia::runtime::protocol::SceneFrameMessageName);

    const auto scene = celestia::runtime::protocol::deserializeSceneFrame(modelFrame.payload);
    REQUIRE(scene.has_value());
    CHECK(scene->camera.fov == doctest::Approx(40.0));

    const auto rendered = view.handle(modelFrame);
    REQUIRE(rendered.size() == 1);
    CHECK(rendered.front().name == "view.frameRendered");
    CHECK(contains(rendered.front().payload, "cameraFov=40"));
}

TEST_CASE("Step16 Ctrl Backspace clears scene selection through Controller Model and View3D")
{
    celestia::runtime::controller::ControllerService controller("step16-controller-interaction");
    celestia::runtime::model::ModelService model("step16-controller-interaction");
    celestia::runtime::view3d::View3DHost view("step16-controller-interaction");

    controller.handle(runtimeStart(RuntimeRole::Controller));
    model.handle(runtimeStart(RuntimeRole::Model));
    view.handle(runtimeStart(RuntimeRole::View));

    const auto controllerCommands = controller.handle(celestia::runtime::protocol::viewInputEnvelope(
        keyDown("Backspace", "Ctrl"),
        RuntimeRole::View,
        RuntimeRole::Controller));

    REQUIRE(controllerCommands.size() == 1);
    CHECK(controllerCommands.front().kind == RuntimeMessageKind::Command);
    CHECK(controllerCommands.front().targetRole == RuntimeRole::Model);
    CHECK(controllerCommands.front().name == "model.clearSelection");
    CHECK(contains(controllerCommands.front().payload, "view=celestia.view3d.opengl"));
    CHECK(contains(controllerCommands.front().payload, "command=selection.clear"));

    const auto modelFrame = model.handle(controllerCommands.front());
    REQUIRE(modelFrame.kind == RuntimeMessageKind::ViewFrame);
    REQUIRE(modelFrame.name == celestia::runtime::protocol::SceneFrameMessageName);

    const auto scene = celestia::runtime::protocol::deserializeSceneFrame(modelFrame.payload);
    REQUIRE(scene.has_value());
    CHECK(scene->selection.type.empty());
    CHECK(scene->selection.id.empty());

    const auto rendered = view.handle(modelFrame);
    REQUIRE(rendered.size() == 1);
    CHECK(rendered.front().name == "view.frameRendered");
    CHECK(contains(rendered.front().payload, "selectionType="));
    CHECK(contains(rendered.front().payload, "selectionId="));
}

TEST_CASE("Step16 H key selects Sol through Controller Model and View3D")
{
    celestia::runtime::controller::ControllerService controller("step16-controller-interaction");
    celestia::runtime::model::ModelService model("step16-controller-interaction");
    celestia::runtime::view3d::View3DHost view("step16-controller-interaction");

    controller.handle(runtimeStart(RuntimeRole::Controller));
    model.handle(runtimeStart(RuntimeRole::Model));
    view.handle(runtimeStart(RuntimeRole::View));

    const auto controllerCommands = controller.handle(celestia::runtime::protocol::viewInputEnvelope(
        keyDown("H"),
        RuntimeRole::View,
        RuntimeRole::Controller));

    REQUIRE(controllerCommands.size() == 1);
    CHECK(controllerCommands.front().kind == RuntimeMessageKind::Command);
    CHECK(controllerCommands.front().targetRole == RuntimeRole::Model);
    CHECK(controllerCommands.front().name == "model.setSelection");
    CHECK(contains(controllerCommands.front().payload, "type=star"));
    CHECK(contains(controllerCommands.front().payload, "id=celestia:star:Sol"));
    CHECK(contains(controllerCommands.front().payload, "view=celestia.view3d.opengl"));
    CHECK(contains(controllerCommands.front().payload, "command=selection.selectObject"));

    const auto modelFrame = model.handle(controllerCommands.front());
    REQUIRE(modelFrame.kind == RuntimeMessageKind::ViewFrame);
    REQUIRE(modelFrame.name == celestia::runtime::protocol::SceneFrameMessageName);

    const auto scene = celestia::runtime::protocol::deserializeSceneFrame(modelFrame.payload);
    REQUIRE(scene.has_value());
    CHECK(scene->selection.type == "star");
    CHECK(scene->selection.id == "celestia:star:Sol");

    const auto rendered = view.handle(modelFrame);
    REQUIRE(rendered.size() == 1);
    CHECK(rendered.front().name == "view.frameRendered");
    CHECK(contains(rendered.front().payload, "selectionType=star"));
    CHECK(contains(rendered.front().payload, "selectionId=celestia:star:Sol"));
}

TEST_CASE("Step16 C key centers camera on current selection through Controller Model and View3D")
{
    celestia::runtime::controller::ControllerService controller("step16-controller-interaction");
    celestia::runtime::model::ModelService model("step16-controller-interaction");
    celestia::runtime::view3d::View3DHost view("step16-controller-interaction");

    controller.handle(runtimeStart(RuntimeRole::Controller));
    model.handle(runtimeStart(RuntimeRole::Model));
    view.handle(runtimeStart(RuntimeRole::View));

    const auto selectCommands = controller.handle(celestia::runtime::protocol::viewInputEnvelope(
        keyDown("H"),
        RuntimeRole::View,
        RuntimeRole::Controller));
    REQUIRE(selectCommands.size() == 1);
    model.handle(selectCommands.front());

    const auto centerCommands = controller.handle(celestia::runtime::protocol::viewInputEnvelope(
        keyDown("C"),
        RuntimeRole::View,
        RuntimeRole::Controller));

    REQUIRE(centerCommands.size() == 1);
    CHECK(centerCommands.front().kind == RuntimeMessageKind::Command);
    CHECK(centerCommands.front().targetRole == RuntimeRole::Model);
    CHECK(centerCommands.front().name == "model.centerSelection");
    CHECK(contains(centerCommands.front().payload, "target=currentSelection"));
    CHECK(contains(centerCommands.front().payload, "view=celestia.view3d.opengl"));
    CHECK(contains(centerCommands.front().payload, "command=camera.center"));

    const auto modelFrame = model.handle(centerCommands.front());
    REQUIRE(modelFrame.kind == RuntimeMessageKind::ViewFrame);
    REQUIRE(modelFrame.name == celestia::runtime::protocol::SceneFrameMessageName);

    const auto scene = celestia::runtime::protocol::deserializeSceneFrame(modelFrame.payload);
    REQUIRE(scene.has_value());
    CHECK(scene->selection.id == "celestia:star:Sol");
    CHECK(scene->camera.position[2] == doctest::Approx(4.0));
    CHECK(scene->observer.position[2] == doctest::Approx(4.0));

    const auto rendered = view.handle(modelFrame);
    REQUIRE(rendered.size() == 1);
    CHECK(rendered.front().name == "view.frameRendered");
    CHECK(contains(rendered.front().payload, "cameraPositionZ=4"));
    CHECK(contains(rendered.front().payload, "selectionId=celestia:star:Sol"));
}

TEST_CASE("Step16 Left key orbits camera around current selection through Controller Model and View3D")
{
    celestia::runtime::controller::ControllerService controller("step16-controller-interaction");
    celestia::runtime::model::ModelService model("step16-controller-interaction");
    celestia::runtime::view3d::View3DHost view("step16-controller-interaction");

    controller.handle(runtimeStart(RuntimeRole::Controller));
    model.handle(runtimeStart(RuntimeRole::Model));
    view.handle(runtimeStart(RuntimeRole::View));

    const auto selectCommands = controller.handle(celestia::runtime::protocol::viewInputEnvelope(
        keyDown("H"),
        RuntimeRole::View,
        RuntimeRole::Controller));
    REQUIRE(selectCommands.size() == 1);
    model.handle(selectCommands.front());

    const auto orbitCommands = controller.handle(celestia::runtime::protocol::viewInputEnvelope(
        keyDown("Left"),
        RuntimeRole::View,
        RuntimeRole::Controller));

    REQUIRE(orbitCommands.size() == 1);
    CHECK(orbitCommands.front().kind == RuntimeMessageKind::Command);
    CHECK(orbitCommands.front().targetRole == RuntimeRole::Model);
    CHECK(orbitCommands.front().name == "model.orbitCamera");
    CHECK(contains(orbitCommands.front().payload, "target=currentSelection"));
    CHECK(contains(orbitCommands.front().payload, "yawDegrees=-15"));
    CHECK(contains(orbitCommands.front().payload, "view=celestia.view3d.opengl"));
    CHECK(contains(orbitCommands.front().payload, "command=camera.orbit"));

    const auto modelFrame = model.handle(orbitCommands.front());
    REQUIRE(modelFrame.kind == RuntimeMessageKind::ViewFrame);
    REQUIRE(modelFrame.name == celestia::runtime::protocol::SceneFrameMessageName);

    const auto scene = celestia::runtime::protocol::deserializeSceneFrame(modelFrame.payload);
    REQUIRE(scene.has_value());
    CHECK(scene->selection.id == "celestia:star:Sol");
    CHECK(scene->camera.orientation[0] == doctest::Approx(0.0));
    CHECK(scene->camera.orientation[1] == doctest::Approx(-0.13052619222005157));
    CHECK(scene->camera.orientation[2] == doctest::Approx(0.0));
    CHECK(scene->camera.orientation[3] == doctest::Approx(0.9914448613738104));

    const auto rendered = view.handle(modelFrame);
    REQUIRE(rendered.size() == 1);
    CHECK(rendered.front().name == "view.frameRendered");
    CHECK(contains(rendered.front().payload, "cameraOrientationY=-0.130526"));
    CHECK(contains(rendered.front().payload, "cameraOrientationW=0.991445"));
    CHECK(contains(rendered.front().payload, "selectionId=celestia:star:Sol"));
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
