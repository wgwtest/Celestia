#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <celestia/viewproviders/debug2dprovider.h>
#include <celestia/viewproviders/view3dprovider.h>
#include <celruntime/runtimecomposition.h>
#include <celruntime/viewframe.h>

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

celestia::runtime::ViewFrame
makeDebugFrame()
{
    celestia::runtime::ViewFrame frame;
    frame.time = 2451545.0;

    celestia::runtime::ViewFrameSelection selection;
    selection.type = "body";
    selection.id = "Earth";
    selection.positionKm = { 1.0, 2.0, 3.0 };
    selection.visible = true;
    selection.clickable = true;
    frame.selections.push_back(selection);

    return frame;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step5 debug 2D view contract");

TEST_CASE("debug 2D provider consumes ViewFrame through ViewRuntime")
{
    const auto header = readSourceFile("src/celestia/viewproviders/debug2dprovider.h");
    CHECK_FALSE(contains(header, "Renderer"));
    CHECK_FALSE(contains(header, "render.h"));
    CHECK_FALSE(contains(header, "meshmanager.h"));
    CHECK_FALSE(contains(header, "texmanager.h"));

    auto provider = celestia::viewproviders::makeDebug2DViewProvider();
    CHECK(provider.id == "celestia.view2d.debug");
    CHECK(provider.dimension == "2d");
    CHECK(provider.create != nullptr);

    auto runtime = provider.create();
    REQUIRE(runtime != nullptr);
    CHECK(runtime->id() == "celestia.view2d.debug");

    runtime->present(makeDebugFrame());
    CHECK(runtime->lastFrameSummary() == "time=2451545 selections=1 selected=Earth type=body visible=true clickable=true");
}

TEST_CASE("same RuntimeComposition can select 3D or debug 2D providers")
{
    celestia::runtime::RuntimeComposition composition;
    CHECK(composition.viewProviders().registerProvider(celestia::viewproviders::makeOpenGLViewProvider()));
    CHECK(composition.viewProviders().registerProvider(celestia::viewproviders::makeDebug2DViewProvider()));

    auto runtime3d = composition.viewProviders().create("celestia.view3d.opengl");
    REQUIRE(runtime3d != nullptr);
    CHECK(runtime3d->id() == "celestia.view3d.opengl");

    auto runtime2d = composition.viewProviders().create("celestia.view2d.debug");
    REQUIRE(runtime2d != nullptr);
    CHECK(runtime2d->id() == "celestia.view2d.debug");
}

TEST_CASE("debug 2D provider is isolated from the 3D provider target")
{
    const auto providerCmake = readSourceFile("src/celestia/viewproviders/CMakeLists.txt");
    CHECK(contains(providerCmake, "add_library(celestia_viewprovider_3d OBJECT"));
    CHECK(contains(providerCmake, "add_library(celestia_viewprovider_debug2d OBJECT"));
    CHECK(contains(providerCmake, "target_link_libraries(celestia_viewprovider_debug2d PRIVATE celestia_runtime)"));
    CHECK_FALSE(contains(providerCmake, "target_link_libraries(celestia_viewprovider_debug2d PRIVATE celestia_runtime celestia_view3d)"));

    const auto appCmake = readSourceFile("src/celestia/CMakeLists.txt");
    CHECK(contains(appCmake, "$<TARGET_OBJECTS:celestia_viewprovider_3d>"));
    CHECK(contains(appCmake, "$<TARGET_OBJECTS:celestia_viewprovider_debug2d>"));
}

TEST_CASE("CelestiaCore registers the built-in debug 2D provider")
{
    const auto coreSource = readSourceFile("src/celestia/celestiacore.cpp");
    CHECK(contains(coreSource, "makeDebug2DViewProvider()"));
}

TEST_SUITE_END();
