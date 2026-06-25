#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

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

void
checkContains(std::string_view text, std::string_view token)
{
    CAPTURE(token);
    CHECK(contains(text, token));
}

void
checkNoToken(std::string_view text, std::string_view token)
{
    CAPTURE(token);
    CHECK_FALSE(contains(text, token));
}

void
checkPathExists(std::string_view relativePath)
{
    CAPTURE(relativePath);
    CHECK(std::filesystem::exists(repoPath(relativePath)));
}

void
checkDirectoryExists(std::string_view relativePath)
{
    CAPTURE(relativePath);
    CHECK(std::filesystem::is_directory(repoPath(relativePath)));
}

void
checkPathMissing(std::string_view relativePath)
{
    CAPTURE(relativePath);
    CHECK_FALSE(std::filesystem::exists(repoPath(relativePath)));
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step4 directory contract");

TEST_CASE("celengine ownership directories exist")
{
    checkDirectoryExists("src/celengine/model");
    checkDirectoryExists("src/celengine/controller");
    checkDirectoryExists("src/celengine/adapter");
    checkDirectoryExists("src/celengine/view3d");
    checkDirectoryExists("src/celengine/legacy");
}

TEST_CASE("representative celengine sources moved out of the root")
{
    checkPathExists("src/celengine/model/body.cpp");
    checkPathExists("src/celengine/model/body.h");
    checkPathExists("src/celengine/model/location.gperf");
    checkPathExists("src/celengine/controller/simulation.cpp");
    checkPathExists("src/celengine/controller/simulation.h");
    checkPathExists("src/celengine/adapter/selectionpicker.cpp");
    checkPathExists("src/celengine/adapter/selectionpicker.h");
    checkPathExists("src/celengine/adapter/solarsys.gperf");
    checkPathExists("src/celengine/view3d/render.cpp");
    checkPathExists("src/celengine/view3d/render.h");
    checkPathExists("src/celengine/legacy/galaxy.cpp");
    checkPathExists("src/celengine/legacy/galaxy.h");

    checkPathMissing("src/celengine/body.cpp");
    checkPathMissing("src/celengine/simulation.cpp");
    checkPathMissing("src/celengine/selectionpicker.cpp");
    checkPathMissing("src/celengine/render.cpp");
    checkPathMissing("src/celengine/galaxy.cpp");
}

TEST_CASE("celengine CMake buckets use directory-qualified paths")
{
    const auto cmake = readSourceFile("src/celengine/CMakeLists.txt");

    checkContains(cmake, "model/body.cpp");
    checkContains(cmake, "controller/simulation.cpp");
    checkContains(cmake, "adapter/selectionpicker.cpp");
    checkContains(cmake, "view3d/render.cpp");
    checkContains(cmake, "legacy/galaxy.cpp");
    checkContains(cmake, "gperf_add_table(celestia_model model/location.gperf model/location.cpp 4)");
    checkContains(cmake, "gperf_add_table(celestia_view_adapter adapter/solarsys.gperf adapter/solarsys.cpp 4)");

    checkNoToken(cmake, "\n  body.cpp");
    checkNoToken(cmake, "\n  simulation.cpp");
    checkNoToken(cmake, "\n  selectionpicker.cpp");
    checkNoToken(cmake, "\n  render.cpp");
    checkNoToken(cmake, "\n  galaxy.cpp");
}

TEST_CASE("Step5 removes public celengine forwarding headers after include migration")
{
    checkPathMissing("src/celengine/body.h");
    checkPathMissing("src/celengine/simulation.h");
    checkPathMissing("src/celengine/selectionpicker.h");
    checkPathMissing("src/celengine/render.h");
    checkPathMissing("src/celengine/galaxy.h");
}

TEST_CASE("celrender renderer helpers live under view3d")
{
    checkDirectoryExists("src/celrender/view3d");
    checkDirectoryExists("src/celrender/view3d/gl");
    checkPathExists("src/celrender/view3d/ringrenderer.cpp");
    checkPathExists("src/celrender/view3d/ringrenderer.h");
    checkPathExists("src/celrender/view3d/gl/binder.cpp");
    checkPathExists("src/celrender/view3d/gl/binder.h");

    checkPathMissing("src/celrender/ringrenderer.cpp");
    checkPathMissing("src/celrender/gl/binder.cpp");

    const auto cmake = readSourceFile("src/celrender/CMakeLists.txt");
    checkContains(cmake, "view3d/ringrenderer.cpp");
    checkContains(cmake, "view3d/gl/binder.cpp");
}

TEST_SUITE_END();
