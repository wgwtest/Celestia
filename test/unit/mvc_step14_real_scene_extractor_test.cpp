#include <doctest.h>

#include <filesystem>
#include <string>
#include <string_view>

#include <celruntime/model/modelsnapshot.h>
#include <celruntime/model/realmodelbackend.h>
#include <celruntime/model/sceneextractor.h>
#include <celruntime/ipc/message.h>
#include <celruntime/protocol/sceneprotocol.h>
#include <celruntime/viewframe.h>
#include <celruntime/viewframecodec.h>

namespace
{

std::filesystem::path
buildRoot()
{
    return std::filesystem::current_path().parent_path().parent_path();
}

std::filesystem::path
contentRoot()
{
    return buildRoot() / "run-full";
}

bool
contains(std::string_view text, std::string_view token)
{
    return text.find(token) != std::string_view::npos;
}

bool
startsWith(std::string_view text, std::string_view prefix)
{
    return text.substr(0, prefix.size()) == prefix;
}

bool
hasResourcePath(const celestia::runtime::protocol::SceneFrame& frame,
                std::string_view relativePath)
{
    for (const auto& resource : frame.resources)
    {
        if (resource.relativePath == relativePath)
            return true;
    }

    return false;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step14 real scene extractor");

TEST_CASE("Step14 SceneExtractor projects real ViewFrame fields into scene.frame")
{
    celestia::runtime::ViewFrame viewFrame;
    viewFrame.frameId = 14;
    viewFrame.time = 2451545.25;
    viewFrame.timeScale = 8.0;
    viewFrame.paused = true;

    viewFrame.camera.positionKm = { 1000.0, 2000.0, 3000.0 };
    viewFrame.camera.orientation = { 0.0, 0.0, 0.0, 1.0 };
    viewFrame.camera.fovDeg = 55.0;
    viewFrame.camera.nearPlaneKm = 0.001;
    viewFrame.camera.farPlaneKm = 1.0e8;

    viewFrame.observer.referenceBodyId = "celestia:body:Sol/Earth";
    viewFrame.observer.frame = "celestia:observer:ecliptical";
    viewFrame.observer.positionKm = { 1000.0, 2000.0, 3000.0 };
    viewFrame.observer.velocityKmPerSec = { 0.1, 0.2, 0.3 };

    celestia::runtime::ViewFrameResource catalog;
    catalog.id = "res:catalog:stars";
    catalog.kind = "catalog";
    catalog.package = "celestia-core";
    catalog.relativePath = "data/stars.dat";
    catalog.required = true;
    viewFrame.resources.push_back(catalog);

    celestia::runtime::ViewFrameResource texture;
    texture.id = "res:texture:earth";
    texture.kind = "texture";
    texture.package = "celestia-core";
    texture.relativePath = "textures/medres/earth.*";
    viewFrame.resources.push_back(texture);

    celestia::runtime::ViewFrameBody earth;
    earth.objectId = "celestia:body:Sol/Earth";
    earth.bodyId = "Sol/Earth";
    earth.name = "Earth";
    earth.positionKm = { 1.0, 2.0, 3.0 };
    earth.radiusKm = 6378.1;
    earth.visible = true;
    earth.diffuseTextureResourceId = "res:texture:earth";
    earth.material = "celestia:body";
    viewFrame.bodies.push_back(earth);

    celestia::runtime::ViewFrameStar sol;
    sol.objectId = "celestia:star:0";
    sol.starId = "0";
    sol.name = "Sol";
    sol.positionKm = { 0.0, 0.0, 0.0 };
    sol.magnitude = -26.74;
    sol.color = { 1.0, 0.95, 0.82 };
    sol.catalogResourceId = "res:catalog:stars";
    viewFrame.stars.push_back(sol);

    celestia::runtime::ViewFrameOrbit orbit;
    orbit.objectId = "celestia:orbit:Sol/Earth";
    orbit.bodyId = "Sol/Earth";
    orbit.visible = true;
    orbit.pointsKm.push_back({ 1.0, 2.0, 3.0 });
    orbit.pointsKm.push_back({ 4.0, 5.0, 6.0 });
    viewFrame.orbits.push_back(orbit);

    celestia::runtime::ViewFrameSelection selection;
    selection.type = "body";
    selection.id = "Sol/Earth";
    selection.positionKm = earth.positionKm;
    selection.visible = true;
    selection.clickable = true;
    viewFrame.selections.push_back(selection);

    const auto serializedViewFrame = celestia::runtime::serializeViewFrame(viewFrame);
    const auto decodedViewFrame = celestia::runtime::deserializeViewFrame(serializedViewFrame);
    REQUIRE(decodedViewFrame.has_value());

    const auto serializedIpcFrame = celestia::runtime::ipc::serializeMessage(
        celestia::runtime::ipc::RuntimeMessage::viewFrame("scene.frame", viewFrame));
    const auto decodedIpcFrame = celestia::runtime::ipc::deserializeMessage(serializedIpcFrame);
    REQUIRE(decodedIpcFrame.has_value());
    CHECK(decodedIpcFrame->frame.resources.size() == 2);
    CHECK(decodedIpcFrame->frame.bodies.size() == 1);
    CHECK(decodedIpcFrame->frame.stars.size() == 1);
    CHECK(decodedIpcFrame->frame.orbits.size() == 1);

    const auto scene = celestia::runtime::model::extractSceneFrame("step14-projection", *decodedViewFrame);

    CHECK(scene.sessionId == "step14-projection");
    CHECK(scene.sequence == 14);
    CHECK(scene.time.julianDayTdb == doctest::Approx(2451545.25));
    CHECK(scene.time.timeScale == doctest::Approx(8.0));
    CHECK(scene.time.paused);
    CHECK(scene.camera.position[0] == doctest::Approx(1000.0));
    CHECK(scene.camera.fov == doctest::Approx(55.0));
    CHECK(scene.observer.frame == "celestia:observer:ecliptical");
    CHECK(scene.observer.referenceBodyId == "celestia:body:Sol/Earth");

    REQUIRE(scene.resources.size() == 2);
    CHECK(hasResourcePath(scene, "data/stars.dat"));
    CHECK(hasResourcePath(scene, "textures/medres/earth.*"));

    REQUIRE(scene.bodies.size() == 1);
    CHECK(scene.bodies.front().objectId == "celestia:body:Sol/Earth");
    CHECK(scene.bodies.front().bodyId == "Sol/Earth");
    CHECK(scene.bodies.front().radius == doctest::Approx(6378.1));
    CHECK(scene.bodies.front().diffuseTexture.id == "res:texture:earth");
    CHECK(scene.bodies.front().material == "celestia:body");

    REQUIRE(scene.stars.size() == 1);
    CHECK(scene.stars.front().objectId == "celestia:star:0");
    CHECK(scene.stars.front().catalogResource.relativePath == "data/stars.dat");

    REQUIRE(scene.orbits.size() == 1);
    CHECK(scene.orbits.front().points.size() == 2);
    CHECK(scene.selection.type == "body");
    CHECK(scene.selection.id == "Sol/Earth");

    CHECK(celestia::runtime::protocol::isValidSceneFrame(scene));
}

TEST_CASE("Step14 RealModelBackend produces a real placeholder-free scene.frame")
{
    REQUIRE(std::filesystem::exists(contentRoot() / "celestia.cfg"));
    REQUIRE(std::filesystem::exists(contentRoot() / "data" / "stars.dat"));
    REQUIRE(std::filesystem::exists(contentRoot() / "data" / "solarsys.ssc"));

    auto backend = celestia::runtime::model::createRealModelBackend();
    REQUIRE(backend != nullptr);

    celestia::runtime::model::RuntimeDataPaths paths;
    paths.dataRoot = contentRoot().string();
    REQUIRE(backend->load(paths));

    backend->setTime(2451545.0);
    const auto viewFrame = backend->snapshot();
    const auto scene = celestia::runtime::model::extractSceneFrame("step14-real", viewFrame);
    const auto payload = celestia::runtime::protocol::serializeSceneFrame(scene);

    CHECK(scene.time.julianDayTdb == doctest::Approx(2451545.0));
    CHECK(scene.time.timeScale == doctest::Approx(1.0));
    CHECK_FALSE(scene.observer.frame.empty());
    CHECK(scene.observer.frame != "step8-synthetic-ecliptic");
    CHECK(scene.camera.fov > 0.0);

    REQUIRE_FALSE(scene.bodies.empty());
    CHECK(startsWith(scene.bodies.front().objectId, "celestia:body:"));
    CHECK(scene.bodies.front().radius > 0.0);
    CHECK(scene.bodies.front().material != "step8-placeholder");

    REQUIRE_FALSE(scene.stars.empty());
    CHECK(startsWith(scene.stars.front().objectId, "celestia:star:"));
    CHECK(scene.stars.front().catalogResource.relativePath == "data/stars.dat");

    CHECK(hasResourcePath(scene, "data/stars.dat"));
    CHECK(hasResourcePath(scene, "data/solarsys.ssc"));
    CHECK_FALSE(contains(payload, "placeholder"));
    CHECK_FALSE(contains(payload, "Synthetic"));
    CHECK_FALSE(contains(payload, "0x"));
    CHECK_FALSE(contains(payload, ":\\"));
    CHECK(celestia::runtime::protocol::isValidSceneFrame(scene));
}

TEST_SUITE_END();
