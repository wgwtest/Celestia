#include <doctest.h>

#include <algorithm>
#include <string>
#include <vector>

#include <celruntime/viewplugin/viewpluginregistry.h>

namespace
{

bool
hasId(const std::vector<std::string>& ids, const std::string& id)
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step9 view plugin registry");

TEST_CASE("builtin View plugin registry exposes debug2D and OpenGL3D manifests")
{
    const auto registry = celestia::runtime::viewplugin::builtinViewPluginRegistry();
    const auto ids = registry.viewIds();

    CHECK(hasId(ids, "celestia.view2d.debug"));
    CHECK(hasId(ids, "celestia.view3d.opengl"));

    const auto* view2d = registry.find("celestia.view2d.debug");
    REQUIRE(view2d != nullptr);
    CHECK(view2d->kind == "view2d");
    CHECK(view2d->supports("view.frame"));

    const auto* view3d = registry.find("celestia.view3d.opengl");
    REQUIRE(view3d != nullptr);
    CHECK(view3d->kind == "view3d");
    CHECK(view3d->supports("scene.frame"));
    CHECK(view3d->supports("view.input"));
    CHECK(view3d->supports("opengl"));
}

TEST_SUITE_END();
