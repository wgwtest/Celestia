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

std::string
readSourceFile(std::string_view relativePath)
{
    std::ifstream input(sourceRoot() / relativePath);
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

TEST_SUITE_BEGIN("MVC Step23 resource boundary");

TEST_CASE("resource path indexes are declared outside View3D")
{
    const auto textureHeader = readSourceFile("src/celengine/resource/texturepaths.h");
    const auto geometryHeader = readSourceFile("src/celengine/resource/geometrypaths.h");

    CHECK(contains(textureHeader, "class TexturePaths"));
    CHECK(contains(textureHeader, "enum class TextureFlags"));
    CHECK(contains(textureHeader, "enum class TextureResolution"));
    CHECK(contains(geometryHeader, "class GeometryPaths"));
    CHECK(contains(geometryHeader, "enum class GeometryHandle"));

    CHECK_FALSE(contains(textureHeader, "celengine/view3d"));
    CHECK_FALSE(contains(textureHeader, "celrender"));
    CHECK_FALSE(contains(textureHeader, "OpenGL"));
    CHECK_FALSE(contains(geometryHeader, "celengine/view3d"));
    CHECK_FALSE(contains(geometryHeader, "celrender"));
    CHECK_FALSE(contains(geometryHeader, "OpenGL"));
}

TEST_CASE("View3D managers include resource indexes instead of owning them")
{
    const auto textureManager = readSourceFile("src/celengine/view3d/texmanager.h");
    const auto meshManager = readSourceFile("src/celengine/view3d/meshmanager.h");

    CHECK(contains(textureManager, "<celengine/resource/texturepaths.h>"));
    CHECK(contains(meshManager, "<celengine/resource/geometrypaths.h>"));

    CHECK_FALSE(contains(textureManager, "class TexturePaths"));
    CHECK_FALSE(contains(meshManager, "class GeometryPaths"));
}

TEST_CASE("RealModelBackend does not include View3D resource manager headers")
{
    const auto source = readSourceFile("src/celruntime/model/realmodelbackend.cpp");

    CHECK(contains(source, "<celengine/resource/geometrypaths.h>"));
    CHECK(contains(source, "<celengine/resource/texturepaths.h>"));
    CHECK_FALSE(contains(source, "<celengine/view3d/meshmanager.h>"));
    CHECK_FALSE(contains(source, "<celengine/view3d/texmanager.h>"));
}

TEST_CASE("resource object library is wired into unified and headless builds")
{
    const auto celengine = readSourceFile("src/celengine/CMakeLists.txt");
    const auto celestia = readSourceFile("src/celestia/CMakeLists.txt");

    CHECK(contains(celengine, "CELESTIA_RESOURCE_SOURCES"));
    CHECK(contains(celengine, "add_library(celestia_resource OBJECT"));
    CHECK(contains(celestia, "$<TARGET_OBJECTS:celestia_resource>"));
}

TEST_SUITE_END();
