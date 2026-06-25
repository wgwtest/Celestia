#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <celengine/adapter/sceneviewmodel.h>
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

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step5 view frame contract");

TEST_CASE("view frame DTO is renderer-free and consumable without a renderer")
{
    const auto header = readSourceFile("src/celruntime/viewframe.h");
    CHECK_FALSE(contains(header, "Renderer"));
    CHECK_FALSE(contains(header, "render.h"));
    CHECK_FALSE(contains(header, "meshmanager.h"));
    CHECK_FALSE(contains(header, "texmanager.h"));
    CHECK_FALSE(contains(header, "shadermanager.h"));

    celestia::runtime::ViewFrame frame;
    frame.time = 2451545.0;

    celestia::runtime::ViewFrameSelection selection;
    selection.type = "body";
    selection.id = "Earth";
    selection.positionKm = { 1.0, 2.0, 3.0 };
    selection.visible = true;
    selection.clickable = true;
    frame.selections.push_back(selection);

    REQUIRE(frame.selections.size() == 1);
    CHECK(frame.selections.front().type == "body");
    CHECK(frame.selections.front().id == "Earth");
    CHECK(frame.selections.front().positionKm[2] == 3.0);
    CHECK(frame.selections.front().visible);
    CHECK(frame.selections.front().clickable);
}

TEST_CASE("SceneViewModel exposes the runtime ViewFrame contract")
{
    const auto sceneViewModelHeader = readSourceFile("src/celengine/adapter/sceneviewmodel.h");
    CHECK(contains(sceneViewModelHeader, "#include <celruntime/viewframe.h>"));
    CHECK(contains(sceneViewModelHeader, "using SceneViewSnapshot = celestia::runtime::ViewFrame"));
    CHECK(contains(sceneViewModelHeader, "using SceneSelectionSnapshot = celestia::runtime::ViewFrameSelection"));
    CHECK_FALSE(contains(sceneViewModelHeader, "selection.h"));
    CHECK_FALSE(contains(sceneViewModelHeader, "Eigen/Core"));
}

TEST_CASE("runtime target owns the ViewFrame contract")
{
    const auto runtimeCmake = readSourceFile("src/celruntime/CMakeLists.txt");
    CHECK(contains(runtimeCmake, "viewframe.h"));
}

TEST_SUITE_END();
