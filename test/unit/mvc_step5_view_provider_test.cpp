#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

#include <celestia/viewproviders/view3dprovider.h>
#include <celruntime/viewproviderregistry.h>

namespace
{

std::filesystem::path
sourceRoot()
{
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
}

std::filesystem::path
repoPath(std::string_view relativePath)
{
    return sourceRoot() / std::filesystem::path(relativePath);
}

std::string
readSourceFile(std::string_view relativePath)
{
    std::ifstream input(repoPath(relativePath));
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

TEST_SUITE_BEGIN("MVC Step5 view provider contract");

TEST_CASE("OpenGL 3D view provider registers without exposing renderer in public API")
{
    const auto header = readSourceFile("src/celestia/viewproviders/view3dprovider.h");
    CHECK_FALSE(contains(header, "Renderer"));
    CHECK_FALSE(contains(header, "render.h"));
    CHECK_FALSE(contains(header, "meshmanager.h"));
    CHECK_FALSE(contains(header, "texmanager.h"));

    auto provider = celestia::viewproviders::makeOpenGLViewProvider();
    CHECK(provider.id == "celestia.view3d.opengl");
    CHECK(provider.dimension == "3d");
    CHECK(provider.create != nullptr);

    celestia::runtime::ViewProviderRegistry registry;
    CHECK(registry.registerProvider(std::move(provider)));

    const auto* registered = registry.find("celestia.view3d.opengl");
    REQUIRE(registered != nullptr);
    CHECK(registered->displayName == "Celestia OpenGL 3D View");

    auto runtime = registry.create("celestia.view3d.opengl");
    REQUIRE(runtime != nullptr);
    CHECK(runtime->id() == "celestia.view3d.opengl");
}

TEST_CASE("view provider target is linked through the application shell only")
{
    const auto viewProviderCmake = readSourceFile("src/celestia/viewproviders/CMakeLists.txt");
    CHECK(contains(viewProviderCmake, "add_library(celestia_viewproviders OBJECT"));

    const auto appCmake = readSourceFile("src/celestia/CMakeLists.txt");
    CHECK(contains(appCmake, "$<TARGET_OBJECTS:celestia_viewproviders>"));
}

TEST_SUITE_END();
