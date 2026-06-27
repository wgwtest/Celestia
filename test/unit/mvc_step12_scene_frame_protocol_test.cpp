#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <celruntime/protocol/sceneprotocol.h>

namespace
{

std::filesystem::path
sourceRoot()
{
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
}

std::string
readText(std::string_view relativePath)
{
    std::ifstream input(sourceRoot() / std::filesystem::path(relativePath));
    REQUIRE(input.good());

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

celestia::runtime::protocol::SceneFrame
validMinimalFrame()
{
    celestia::runtime::protocol::SceneFrame frame;
    frame.protocolVersion = celestia::runtime::protocol::SceneFrameProtocolVersion;
    frame.sessionId = "step12-protocol";
    frame.sequence = 1;
    frame.simulationTime = 2451545.0;
    frame.time.julianDayTdb = 2451545.0;
    frame.time.secondsSinceJ2000 = 0.0;
    frame.time.timeScale = 1.0;
    frame.time.paused = false;
    frame.camera.fov = 45.0;
    frame.camera.nearPlane = 0.01;
    frame.camera.farPlane = 1.0e9;
    frame.observer.referenceBodyId = "celestia:body:Sol";
    frame.observer.frame = "celestia:ecliptic";
    return frame;
}

bool
hasIssueForField(const std::vector<celestia::runtime::protocol::SceneFrameValidationIssue>& issues,
                 std::string_view field)
{
    for (const auto& issue : issues)
    {
        if (issue.field.find(field) != std::string::npos)
            return true;
    }

    return false;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step12 scene frame protocol");

TEST_CASE("Step12 SceneFrame vNext exposes protocol version and structured time")
{
    auto frame = validMinimalFrame();

    const auto payload = celestia::runtime::protocol::serializeSceneFrame(frame);
    const auto parsed = celestia::runtime::protocol::deserializeSceneFrame(payload);

    REQUIRE(parsed.has_value());
    CHECK(parsed->protocolVersion == celestia::runtime::protocol::SceneFrameProtocolVersion);
    CHECK(parsed->time.julianDayTdb == doctest::Approx(2451545.0));
    CHECK(parsed->time.secondsSinceJ2000 == doctest::Approx(0.0));
    CHECK(parsed->time.timeScale == doctest::Approx(1.0));
    CHECK_FALSE(parsed->time.paused);
}

TEST_CASE("Step12 SceneFrame vNext round-trips ResourceRef vNext fields")
{
    auto frame = validMinimalFrame();

    celestia::runtime::protocol::ResourceRef resource;
    resource.id = "res:texture:earth";
    resource.kind = "texture";
    resource.package = "celestia-core";
    resource.relativePath = "textures/earth.png";
    resource.contentHash = "sha256:earth";
    resource.dataPlaneKey = "dataplane-earth-texture";
    resource.required = true;
    frame.resources.push_back(resource);

    const auto parsed = celestia::runtime::protocol::deserializeSceneFrame(
        celestia::runtime::protocol::serializeSceneFrame(frame));

    REQUIRE(parsed.has_value());
    REQUIRE(parsed->resources.size() == 1);
    CHECK(parsed->resources.front().id == "res:texture:earth");
    CHECK(parsed->resources.front().dataPlaneKey == "dataplane-earth-texture");
    CHECK(parsed->resources.front().required);
}

TEST_CASE("Step12 SceneFrame vNext supports structured labels")
{
    auto frame = validMinimalFrame();

    celestia::runtime::protocol::LabelRenderState label;
    label.targetObjectId = "celestia:body:Sol/Earth";
    label.text = "Earth";
    label.kind = "body";
    label.visible = true;
    frame.labels.push_back(label);

    const auto parsed = celestia::runtime::protocol::deserializeSceneFrame(
        celestia::runtime::protocol::serializeSceneFrame(frame));

    REQUIRE(parsed.has_value());
    REQUIRE(parsed->labels.size() == 1);
    CHECK(parsed->labels.front().targetObjectId == "celestia:body:Sol/Earth");
    CHECK(parsed->labels.front().text == "Earth");
    CHECK(parsed->labels.front().kind == "body");
    CHECK(parsed->labels.front().visible);
}

TEST_CASE("Step12 SceneFrame vNext supports DSO payload fields")
{
    auto frame = validMinimalFrame();

    celestia::runtime::protocol::DeepSkyRenderState dso;
    dso.objectId = "celestia:dso:M31";
    dso.name = "Andromeda Galaxy";
    dso.dsoType = "galaxy";
    dso.positionKm = { 1.0, 2.0, 3.0 };
    dso.apparentMagnitude = 3.44;
    dso.visible = true;
    dso.catalogResource.id = "res:catalog:dso";
    dso.catalogResource.kind = "catalog";
    dso.catalogResource.package = "celestia-core";
    dso.catalogResource.relativePath = "catalogs/dso.dat";
    frame.deepSkyObjects.push_back(dso);

    const auto parsed = celestia::runtime::protocol::deserializeSceneFrame(
        celestia::runtime::protocol::serializeSceneFrame(frame));

    REQUIRE(parsed.has_value());
    REQUIRE(parsed->deepSkyObjects.size() == 1);
    CHECK(parsed->deepSkyObjects.front().objectId == "celestia:dso:M31");
    CHECK(parsed->deepSkyObjects.front().name == "Andromeda Galaxy");
    CHECK(parsed->deepSkyObjects.front().positionKm[2] == doctest::Approx(3.0));
    CHECK(parsed->deepSkyObjects.front().catalogResource.relativePath == "catalogs/dso.dat");
}

TEST_CASE("Step12 validation accepts valid minimal frame")
{
    const auto issues = celestia::runtime::protocol::validateSceneFrame(validMinimalFrame());
    CHECK(issues.empty());
    CHECK(celestia::runtime::protocol::isValidSceneFrame(validMinimalFrame()));
}

TEST_CASE("Step12 validation rejects process-local resource handles")
{
    auto frame = validMinimalFrame();

    celestia::runtime::protocol::ResourceRef resource;
    resource.id = "bad";
    resource.kind = "texture";
    resource.package = "celestia";
    resource.relativePath = "C:\\textures\\earth.png";
    frame.resources.push_back(resource);

    const auto issues = celestia::runtime::protocol::validateSceneFrame(frame);
    CHECK_FALSE(issues.empty());
    CHECK(hasIssueForField(issues, "resource0.relativePath"));
}

TEST_CASE("Step12 fixtures document valid and invalid scene.frame payloads")
{
    const auto valid = readText("test/fixtures/mvc/scene_frame_vnext_minimal.sceneframe");
    const auto validFrame = celestia::runtime::protocol::deserializeSceneFrame(valid);
    REQUIRE(validFrame.has_value());
    CHECK(celestia::runtime::protocol::isValidSceneFrame(*validFrame));

    const auto solarSystem = readText("test/fixtures/mvc/scene_frame_vnext_solar_system.sceneframe");
    const auto solarSystemFrame = celestia::runtime::protocol::deserializeSceneFrame(solarSystem);
    REQUIRE(solarSystemFrame.has_value());
    CHECK(celestia::runtime::protocol::isValidSceneFrame(*solarSystemFrame));
    CHECK(solarSystemFrame->resources.size() == 2);
    CHECK(solarSystemFrame->bodies.size() == 3);
    CHECK(solarSystemFrame->stars.size() == 1);
    CHECK(solarSystemFrame->orbits.size() == 1);
    CHECK(solarSystemFrame->labels.size() == 2);
    REQUIRE_FALSE(solarSystemFrame->orbits.empty());
    REQUIRE(solarSystemFrame->orbits.front().points.size() == 2);
    CHECK(solarSystemFrame->orbits.front().points[1][0] == doctest::Approx(1.0));

    const auto invalidMissingCamera = readText("test/fixtures/mvc/scene_frame_vnext_invalid_missing_camera.sceneframe");
    const auto missingCameraFrame = celestia::runtime::protocol::deserializeSceneFrame(invalidMissingCamera);
    REQUIRE(missingCameraFrame.has_value());
    const auto missingCameraIssues = celestia::runtime::protocol::validateSceneFrame(*missingCameraFrame);
    CHECK(hasIssueForField(missingCameraIssues, "camera.fov"));

    const auto invalidResource = readText("test/fixtures/mvc/scene_frame_vnext_invalid_absolute_resource.sceneframe");
    const auto invalidResourceFrame = celestia::runtime::protocol::deserializeSceneFrame(invalidResource);
    REQUIRE(invalidResourceFrame.has_value());
    CHECK_FALSE(celestia::runtime::protocol::isValidSceneFrame(*invalidResourceFrame));

    const auto invalidHandle = readText("test/fixtures/mvc/scene_frame_vnext_invalid_process_handle.sceneframe");
    const auto invalidHandleFrame = celestia::runtime::protocol::deserializeSceneFrame(invalidHandle);
    REQUIRE(invalidHandleFrame.has_value());
    const auto handleIssues = celestia::runtime::protocol::validateSceneFrame(*invalidHandleFrame);
    CHECK_FALSE(handleIssues.empty());
}

TEST_CASE("Step12 SceneFrame protocol header stays renderer and OpenGL free")
{
    const auto header = readText("src/celruntime/protocol/sceneprotocol.h");
    CHECK(header.find("Renderer") == std::string::npos);
    CHECK(header.find("OpenGL") == std::string::npos);
    CHECK(header.find("GLuint") == std::string::npos);
    CHECK(header.find("Texture*") == std::string::npos);
    CHECK(header.find("Mesh*") == std::string::npos);
    CHECK(header.find("Simulation*") == std::string::npos);
    CHECK(header.find("Universe*") == std::string::npos);
    CHECK(header.find("CelestiaCore*") == std::string::npos);
}

TEST_CASE("Step12 SceneFrame protocol remains backward-compatible with Step8 frame")
{
    constexpr std::string_view legacyPayload{
        "sessionId=step8-legacy;sequence=42;simulationTime=123.5;"
        "camera.fov=45;camera.nearPlane=0.1;camera.farPlane=1000000;"
        "observer.frame=celestia:ecliptic;labelCount=1;label0=Earth"
    };

    const auto parsed = celestia::runtime::protocol::deserializeSceneFrame(legacyPayload);
    REQUIRE(parsed.has_value());
    CHECK(parsed->protocolVersion == 1);
    CHECK(parsed->simulationTime == doctest::Approx(123.5));
    REQUIRE(parsed->labels.size() == 1);
    CHECK(parsed->labels.front().text == "Earth");
}

TEST_SUITE_END();
