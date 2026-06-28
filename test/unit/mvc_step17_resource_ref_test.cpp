#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>

#include <celruntime/protocol/envelope.h>
#include <celruntime/protocol/sceneprotocol.h>
#include <celruntime/view3d/view3dhost.h>
#include <celruntime/view3d/view3dresources.h>

namespace
{

std::filesystem::path
makeContentRoot()
{
    const auto root = std::filesystem::temp_directory_path() / "celestia-step17-resource-ref";
    std::filesystem::create_directories(root / "textures");
    std::ofstream(root / "textures" / "earth.png").put('x');
    return root;
}

celestia::runtime::protocol::ResourceRef
resource(std::string id, std::string kind, std::string relativePath, bool required)
{
    celestia::runtime::protocol::ResourceRef ref;
    ref.id = std::move(id);
    ref.kind = std::move(kind);
    ref.package = "celestia-core";
    ref.relativePath = std::move(relativePath);
    ref.required = required;
    return ref;
}

celestia::runtime::protocol::RuntimeEnvelope
sceneEnvelope(const celestia::runtime::protocol::SceneFrame& frame)
{
    return celestia::runtime::protocol::sceneFrameEnvelope(
        frame,
        celestia::runtime::protocol::RuntimeRole::Model,
        celestia::runtime::protocol::RuntimeRole::View);
}

bool
contains(std::string_view text, std::string_view token)
{
    return text.find(token) != std::string_view::npos;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step17 ResourceRef and DataPlane");

TEST_CASE("Step17 ResourceRef resolver reports status and stable cache keys")
{
    const auto root = makeContentRoot();

    celestia::runtime::protocol::SceneFrame frame;
    frame.resources.push_back(resource("res:texture:earth", "texture", "textures/earth.png", true));
    frame.resources.push_back(resource("res:texture:missing", "texture", "textures/missing.png", true));
    frame.resources.push_back(resource("res:texture:absolute", "texture", "C:\\temp\\earth.png", true));
    frame.resources.push_back(resource("res:texture:traversal", "texture", "../secret.png", true));

    const auto resolved = celestia::runtime::view3d::resolveSceneResources(frame, root);
    REQUIRE(resolved.size() == 4);

    CHECK(resolved[0].status == celestia::runtime::view3d::View3DResourceStatus::Resolved);
    CHECK(resolved[0].exists);
    CHECK(resolved[0].cacheKey == "celestia-core|texture|textures/earth.png");

    CHECK(resolved[1].status == celestia::runtime::view3d::View3DResourceStatus::MissingRequired);
    CHECK_FALSE(resolved[1].exists);
    CHECK(resolved[1].cacheKey == "celestia-core|texture|textures/missing.png");

    CHECK(resolved[2].status == celestia::runtime::view3d::View3DResourceStatus::Invalid);
    CHECK_FALSE(resolved[2].exists);

    CHECK(resolved[3].status == celestia::runtime::view3d::View3DResourceStatus::Invalid);
    CHECK_FALSE(resolved[3].exists);
}

TEST_CASE("Step17 View3DHost reports missing and invalid resources")
{
    const auto root = makeContentRoot();

    celestia::runtime::protocol::SceneFrame frame;
    frame.sessionId = "step17-resource-host";
    frame.sequence = 17;
    frame.simulationTime = 2451545.0;
    frame.camera.fov = 45.0;
    frame.resources.push_back(resource("res:texture:earth", "texture", "textures/earth.png", true));
    frame.resources.push_back(resource("res:texture:missing", "texture", "textures/missing.png", true));
    frame.resources.push_back(resource("res:texture:absolute", "texture", "C:\\temp\\earth.png", true));

    celestia::runtime::view3d::View3DHostOptions options;
    options.contentRoot = root;
    celestia::runtime::view3d::View3DHost host("step17-resource-host", options);

    const auto responses = host.handle(sceneEnvelope(frame));
    REQUIRE(responses.size() == 3);
    CHECK(responses[0].name == "view.frameRendered");
    CHECK(contains(responses[0].payload, "resourceCount=3"));
    CHECK(contains(responses[0].payload, "resolvedResourceCount=1"));
    CHECK(contains(responses[0].payload, "missingRequiredResourceCount=1"));
    CHECK(contains(responses[0].payload, "invalidResourceCount=1"));
    CHECK(contains(responses[0].payload, "dataPlaneEligibleResourceCount=0"));
    CHECK(responses[1].name == "view.resourceMissing");
    CHECK(contains(responses[1].payload, "id=res:texture:missing"));
    CHECK(responses[2].name == "view.resourceMissing");
    CHECK(contains(responses[2].payload, "id=res:texture:absolute"));
}

TEST_SUITE_END();
