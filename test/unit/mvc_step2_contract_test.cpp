#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace
{

std::filesystem::path sourceRoot()
{
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
}

std::string readSourceFile(const std::filesystem::path& relativePath)
{
    std::ifstream input(sourceRoot() / relativePath);
    REQUIRE(input.good());

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool contains(const std::string& text, const std::string& token)
{
    return text.find(token) != std::string::npos;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step2 contract");

TEST_CASE("model implementation files do not call concrete render asset sidecars")
{
    const auto body = readSourceFile("src/celengine/body.cpp");
    const auto star = readSourceFile("src/celengine/star.cpp");
    const auto nebula = readSourceFile("src/celengine/nebula.cpp");

    CHECK_FALSE(contains(body, "BodyRenderAssets::"));
    CHECK_FALSE(contains(star, "StarRenderAssets::"));
    CHECK_FALSE(contains(nebula, "NebulaRenderAssets::"));
}

TEST_CASE("model implementation files do not include concrete view resource headers")
{
    const std::filesystem::path files[] = {
        "src/celengine/body.cpp",
        "src/celengine/star.cpp",
        "src/celengine/nebula.cpp",
    };

    for (const auto& file : files)
    {
        const auto source = readSourceFile(file);
        CHECK_FALSE(contains(source, "bodyrenderassets.h"));
        CHECK_FALSE(contains(source, "starrenderassets.h"));
        CHECK_FALSE(contains(source, "nebularenderassets.h"));
        CHECK_FALSE(contains(source, "meshmanager.h"));
        CHECK_FALSE(contains(source, "render.h"));
    }
}

TEST_CASE("star model header does not friend concrete render asset adapter")
{
    const auto starHeader = readSourceFile("src/celengine/star.h");

    CHECK_FALSE(contains(starHeader, "class StarRenderAssets"));
    CHECK_FALSE(contains(starHeader, "friend class StarRenderAssets"));
}

TEST_CASE("deep sky model API does not require geometry path resources")
{
    const auto dsoHeader = readSourceFile("src/celengine/deepskyobj.h");
    const auto nebulaHeader = readSourceFile("src/celengine/nebula.h");

    CHECK_FALSE(contains(dsoHeader, "GeometryPaths"));
    CHECK_FALSE(contains(nebulaHeader, "GeometryPaths"));
}

TEST_SUITE_END();
