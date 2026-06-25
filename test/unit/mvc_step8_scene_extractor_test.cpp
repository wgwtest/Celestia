#include <doctest.h>

#include <celruntime/model/modelservice.h>
#include <celruntime/model/sceneextractor.h>
#include <celruntime/protocol/envelope.h>
#include <celruntime/protocol/lifecycle.h>
#include <celruntime/protocol/sceneprotocol.h>
#include <celruntime/viewframe.h>

namespace
{

using celestia::runtime::protocol::RuntimeEnvelope;
using celestia::runtime::protocol::RuntimeMessageKind;
using celestia::runtime::protocol::RuntimeRole;

RuntimeEnvelope
modelCommand(std::string name, std::string payload = {})
{
    RuntimeEnvelope envelope;
    envelope.sessionId = "step8-scene-extractor";
    envelope.sourceRole = RuntimeRole::Controller;
    envelope.targetRole = RuntimeRole::Model;
    envelope.kind = RuntimeMessageKind::Command;
    envelope.name = std::move(name);
    envelope.payload = std::move(payload);
    return envelope;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step8 scene extractor");

TEST_CASE("SceneExtractor creates a minimal renderer-free scene frame")
{
    celestia::runtime::ViewFrame viewFrame;
    viewFrame.frameId = 7;
    viewFrame.time = 123.5;

    celestia::runtime::ViewFrameSelection earth;
    earth.type = "body";
    earth.id = "Earth";
    earth.positionKm = { 1.0, 2.0, 3.0 };
    earth.visible = true;
    viewFrame.selections.push_back(std::move(earth));

    const auto scene = celestia::runtime::model::extractSceneFrame("step8-scene-extractor", viewFrame);

    CHECK(scene.sessionId == "step8-scene-extractor");
    CHECK(scene.sequence == 7);
    CHECK(scene.simulationTime == doctest::Approx(123.5));
    CHECK(scene.renderSettings.showStars);
    CHECK(scene.renderSettings.showOrbits);
    REQUIRE_FALSE(scene.resources.empty());
    REQUIRE_FALSE(scene.bodies.empty());
    REQUIRE_FALSE(scene.stars.empty());
    REQUIRE_FALSE(scene.orbits.empty());
    CHECK(scene.bodies.front().bodyId == "Earth");
    CHECK(scene.bodies.front().visible);
    CHECK(scene.resources.front().relativePath.find(":\\") == std::string::npos);
}

TEST_CASE("ModelService can emit scene.frame for the 3D view without breaking 2D view.frame")
{
    celestia::runtime::model::ModelService service("step8-scene-extractor");

    auto start = celestia::runtime::protocol::lifecycle(RuntimeRole::Launcher,
                                                       RuntimeRole::Model,
                                                       celestia::runtime::protocol::RuntimeStart);
    start.sessionId = "step8-scene-extractor";
    service.handle(start);

    const auto twoD = service.handle(modelCommand("model.step", "dt=0.5"));
    REQUIRE(twoD.kind == RuntimeMessageKind::ViewFrame);
    CHECK(twoD.name == "view.frame");

    const auto threeD = service.handle(modelCommand("model.step", "dt=0.5;view=celestia.view3d.opengl"));
    REQUIRE(threeD.kind == RuntimeMessageKind::ViewFrame);
    REQUIRE(threeD.name == celestia::runtime::protocol::SceneFrameMessageName);

    const auto scene = celestia::runtime::protocol::deserializeSceneFrame(threeD.payload);
    REQUIRE(scene.has_value());
    CHECK(scene->sessionId == "step8-scene-extractor");
    CHECK(scene->sequence >= 2);
    CHECK(scene->simulationTime == doctest::Approx(1.0));
    CHECK(scene->camera.fov > 0.0);
    CHECK(scene->renderSettings.showStars);
    REQUIRE_FALSE(scene->resources.empty());
    REQUIRE_FALSE(scene->bodies.empty());
    CHECK(scene->resources.front().relativePath.find(":\\") == std::string::npos);
}

TEST_SUITE_END();
