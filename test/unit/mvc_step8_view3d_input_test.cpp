#include <doctest.h>

#include <string>

#include <celruntime/controller/controllerservice.h>
#include <celruntime/model/modelservice.h>
#include <celruntime/protocol/envelope.h>
#include <celruntime/protocol/lifecycle.h>
#include <celruntime/protocol/sceneprotocol.h>
#include <celruntime/protocol/viewinput.h>

namespace
{

using celestia::runtime::protocol::RuntimeEnvelope;
using celestia::runtime::protocol::RuntimeMessageKind;
using celestia::runtime::protocol::RuntimeRole;
using celestia::runtime::protocol::ViewInputEvent;

ViewInputEvent
mouseWheelInput()
{
    ViewInputEvent input;
    input.sessionId = "step8-view3d-input-test";
    input.sequence = 5;
    input.device = "mouse";
    input.action = "MouseWheel";
    input.pointer = { 320.0, 240.0 };
    input.wheel = { 0.0, 1.0 };
    input.modifiers = "Shift";
    return input;
}

RuntimeEnvelope
modelCommand(std::string name, std::string payload = {})
{
    RuntimeEnvelope envelope;
    envelope.sessionId = "step8-view3d-input-test";
    envelope.sourceRole = RuntimeRole::Controller;
    envelope.targetRole = RuntimeRole::Model;
    envelope.kind = RuntimeMessageKind::Command;
    envelope.name = std::move(name);
    envelope.payload = std::move(payload);
    return envelope;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step8 view3d input");

TEST_CASE("View3D input event serializes through RuntimeEnvelope")
{
    const auto envelope = celestia::runtime::protocol::viewInputEnvelope(
        mouseWheelInput(),
        RuntimeRole::View,
        RuntimeRole::Controller);

    CHECK(envelope.kind == RuntimeMessageKind::Event);
    CHECK(envelope.name == celestia::runtime::protocol::ViewInputMessageName);
    CHECK(envelope.sourceRole == RuntimeRole::View);
    CHECK(envelope.targetRole == RuntimeRole::Controller);

    const auto serialized = celestia::runtime::protocol::serializeEnvelope(envelope);
    const auto parsedEnvelope = celestia::runtime::protocol::deserializeEnvelope(serialized);
    REQUIRE(parsedEnvelope.has_value());

    const auto parsedInput = celestia::runtime::protocol::deserializeViewInputEvent(parsedEnvelope->payload);
    REQUIRE(parsedInput.has_value());
    CHECK(parsedInput->sessionId == "step8-view3d-input-test");
    CHECK(parsedInput->sequence == 5);
    CHECK(parsedInput->device == "mouse");
    CHECK(parsedInput->action == "MouseWheel");
    CHECK(parsedInput->pointer[0] == doctest::Approx(320.0));
    CHECK(parsedInput->wheel[1] == doctest::Approx(1.0));
    CHECK(parsedInput->modifiers == "Shift");
}

TEST_CASE("ControllerService converts view.input into a model command")
{
    celestia::runtime::controller::ControllerService controller("step8-view3d-input-test");
    const auto response = controller.handle(celestia::runtime::protocol::viewInputEnvelope(
        mouseWheelInput(),
        RuntimeRole::View,
        RuntimeRole::Controller));

    REQUIRE(response.size() == 1);
    CHECK(response.front().kind == RuntimeMessageKind::Command);
    CHECK(response.front().targetRole == RuntimeRole::Model);
    CHECK(response.front().name == "model.setViewInput");
    CHECK(response.front().payload.find("action=MouseWheel") != std::string::npos);
    CHECK(response.front().payload.find("wheelY=1") != std::string::npos);
}

TEST_CASE("ModelService reflects view.input state in the next scene.frame")
{
    celestia::runtime::model::ModelService model("step8-view3d-input-test");

    auto start = celestia::runtime::protocol::lifecycle(RuntimeRole::Launcher,
                                                       RuntimeRole::Model,
                                                       celestia::runtime::protocol::RuntimeStart);
    start.sessionId = "step8-view3d-input-test";
    model.handle(start);

    const auto command = model.handle(modelCommand("model.setViewInput", "action=MouseWheel;key=;pointerX=320;pointerY=240;wheelX=0;wheelY=1"));
    CHECK(command.kind == RuntimeMessageKind::ViewFrame);
    CHECK(command.name == celestia::runtime::protocol::SceneFrameMessageName);

    const auto scene = celestia::runtime::protocol::deserializeSceneFrame(command.payload);
    REQUIRE(scene.has_value());
    REQUIRE_FALSE(scene->labels.empty());

    bool found = false;
    for (const auto& label : scene->labels)
    {
        if (label.text.find("input:MouseWheel") != std::string::npos)
            found = true;
    }
    CHECK(found);
}

TEST_SUITE_END();
