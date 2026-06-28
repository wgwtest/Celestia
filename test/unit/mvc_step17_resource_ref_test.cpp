#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

#include <celruntime/protocol/sceneprotocol.h>
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

TEST_SUITE_END();
