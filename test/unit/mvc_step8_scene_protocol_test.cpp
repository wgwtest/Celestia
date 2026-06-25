#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <celruntime/protocol/envelope.h>
#include <celruntime/protocol/sceneprotocol.h>

namespace
{

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

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step8 scene protocol");

TEST_CASE("SceneFrame serializes through RuntimeEnvelope without renderer state")
{
    using celestia::runtime::protocol::BodyRenderState;
    using celestia::runtime::protocol::CameraState;
    using celestia::runtime::protocol::ResourceRef;
    using celestia::runtime::protocol::RuntimeMessageKind;
    using celestia::runtime::protocol::RuntimeRole;
    using celestia::runtime::protocol::SceneFrame;

    SceneFrame frame;
    frame.sessionId = "step8-scene-protocol";
    frame.sequence = 42;
    frame.simulationTime = 123.5;
    frame.camera.position = { 1.0, 2.0, 3.0 };
    frame.camera.orientation = { 1.0, 0.0, 0.0, 0.0 };
    frame.camera.fov = 45.0;
    frame.camera.nearPlane = 0.1;
    frame.camera.farPlane = 1000000.0;
    frame.renderSettings.showStars = true;
    frame.renderSettings.showOrbits = true;
    frame.renderSettings.showLabels = false;

    ResourceRef mesh;
    mesh.kind = "mesh";
    mesh.package = "celestia";
    mesh.relativePath = "models/earth.cmod";
    mesh.contentHash = "mesh-hash";

    ResourceRef texture;
    texture.kind = "texture";
    texture.package = "celestia";
    texture.relativePath = "textures/earth.png";
    texture.contentHash = "texture-hash";

    BodyRenderState earth;
    earth.bodyId = "Sol/Earth";
    earth.name = "Earth";
    earth.transform = {
        1.0, 0.0, 0.0, 1000.0,
        0.0, 1.0, 0.0, 2000.0,
        0.0, 0.0, 1.0, 3000.0,
        0.0, 0.0, 0.0, 1.0,
    };
    earth.radius = 6371.0;
    earth.visible = true;
    earth.meshResource = mesh;
    earth.diffuseTexture = texture;
    frame.bodies.push_back(earth);
    frame.resources.push_back(mesh);
    frame.resources.push_back(texture);

    const auto envelope = celestia::runtime::protocol::sceneFrameEnvelope(
        frame,
        RuntimeRole::Model,
        RuntimeRole::View);

    CHECK(envelope.sessionId == "step8-scene-protocol");
    CHECK(envelope.kind == RuntimeMessageKind::ViewFrame);
    CHECK(envelope.name == celestia::runtime::protocol::SceneFrameMessageName);

    const auto serializedEnvelope = celestia::runtime::protocol::serializeEnvelope(envelope);
    const auto parsedEnvelope = celestia::runtime::protocol::deserializeEnvelope(serializedEnvelope);
    REQUIRE(parsedEnvelope.has_value());

    const auto parsedFrame = celestia::runtime::protocol::deserializeSceneFrame(parsedEnvelope->payload);
    REQUIRE(parsedFrame.has_value());
    CHECK(parsedFrame->sessionId == "step8-scene-protocol");
    CHECK(parsedFrame->sequence == 42);
    CHECK(parsedFrame->simulationTime == doctest::Approx(123.5));
    CHECK(parsedFrame->camera.position[0] == doctest::Approx(1.0));
    CHECK(parsedFrame->camera.fov == doctest::Approx(45.0));
    REQUIRE(parsedFrame->bodies.size() == 1);
    CHECK(parsedFrame->bodies.front().bodyId == "Sol/Earth");
    CHECK(parsedFrame->bodies.front().meshResource.relativePath == "models/earth.cmod");
    CHECK(parsedFrame->bodies.front().diffuseTexture.relativePath == "textures/earth.png");
    REQUIRE(parsedFrame->resources.size() == 2);
    CHECK(parsedFrame->resources.back().contentHash == "texture-hash");
}

TEST_CASE("SceneFrame protocol header stays renderer and OpenGL free")
{
    const auto header = readSourceFile("src/celruntime/protocol/sceneprotocol.h");
    CHECK_FALSE(contains(header, "Renderer"));
    CHECK_FALSE(contains(header, "OpenGL"));
    CHECK_FALSE(contains(header, "GLuint"));
    CHECK_FALSE(contains(header, "Texture*"));
    CHECK_FALSE(contains(header, "Mesh*"));
    CHECK_FALSE(contains(header, "Simulation*"));
    CHECK_FALSE(contains(header, "Universe*"));
    CHECK_FALSE(contains(header, "CelestiaCore*"));
    CHECK_FALSE(contains(header, "render.h"));
    CHECK_FALSE(contains(header, "glsupport.h"));
}

TEST_SUITE_END();
