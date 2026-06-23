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

TEST_SUITE_BEGIN("MVC boundary");

TEST_CASE("Simulation controller header does not expose view adapter API")
{
    const auto simulationHeader = readSourceFile("src/celengine/simulation.h");

    CHECK_FALSE(contains(simulationHeader, "class Renderer"));
    CHECK_FALSE(contains(simulationHeader, "RenderFlags"));
    CHECK_FALSE(contains(simulationHeader, "void render("));
    CHECK_FALSE(contains(simulationHeader, "pickObject("));
}

TEST_CASE("Universe model header does not expose picking or view resources")
{
    const auto universeHeader = readSourceFile("src/celengine/universe.h");

    CHECK_FALSE(contains(universeHeader, "GeometryManager"));
    CHECK_FALSE(contains(universeHeader, "RenderFlags"));
    CHECK_FALSE(contains(universeHeader, "Selection pick("));
    CHECK_FALSE(contains(universeHeader, "pickPlanet("));
    CHECK_FALSE(contains(universeHeader, "pickStar("));
    CHECK_FALSE(contains(universeHeader, "pickDeepSkyObject("));
}

TEST_CASE("Body feature manager header does not expose geometry adapter API")
{
    const auto bodyHeader = readSourceFile("src/celengine/body.h");

    CHECK_FALSE(contains(bodyHeader, "GeometryManager"));
    CHECK_FALSE(contains(bodyHeader, "computeLocations("));
}

TEST_CASE("Body model header does not expose primary render assets")
{
    const auto bodyHeader = readSourceFile("src/celengine/body.h");

    CHECK_FALSE(contains(bodyHeader, "meshmanager.h"));
    CHECK_FALSE(contains(bodyHeader, "getGeometry()"));
    CHECK_FALSE(contains(bodyHeader, "setGeometry("));
    CHECK_FALSE(contains(bodyHeader, "getGeometryOrientation("));
    CHECK_FALSE(contains(bodyHeader, "setGeometryOrientation("));
    CHECK_FALSE(contains(bodyHeader, "getGeometryScale("));
    CHECK_FALSE(contains(bodyHeader, "setGeometryScale("));
    CHECK_FALSE(contains(bodyHeader, "getSurface()"));
    CHECK_FALSE(contains(bodyHeader, "setSurface("));
    CHECK_FALSE(contains(bodyHeader, "getAlternateSurface("));
    CHECK_FALSE(contains(bodyHeader, "addAlternateSurface("));
    CHECK_FALSE(contains(bodyHeader, "std::unique_ptr<Surface>"));
    CHECK_FALSE(contains(bodyHeader, "surface.h"));
    CHECK_FALSE(contains(bodyHeader, "GeometryHandle geometry"));
    CHECK_FALSE(contains(bodyHeader, "Surface surface"));
    CHECK_FALSE(contains(bodyHeader, "TextureHandle"));
    CHECK_FALSE(contains(bodyHeader, "texture{"));
}

TEST_CASE("Star model header does not expose render assets")
{
    const auto starHeader = readSourceFile("src/celengine/star.h");

    CHECK_FALSE(contains(starHeader, "meshmanager.h"));
    CHECK_FALSE(contains(starHeader, "TextureHandle"));
    CHECK_FALSE(contains(starHeader, "GeometryHandle"));
    CHECK_FALSE(contains(starHeader, "getTexture()"));
    CHECK_FALSE(contains(starHeader, "setTexture("));
    CHECK_FALSE(contains(starHeader, "getGeometry()"));
    CHECK_FALSE(contains(starHeader, "setGeometry("));
    CHECK_FALSE(contains(starHeader, "StarTextureSet"));
}

TEST_CASE("Deep sky model headers do not expose render mask API")
{
    const std::filesystem::path headers[] = {
        "src/celengine/deepskyobj.h",
        "src/celengine/galaxy.h",
        "src/celengine/globular.h",
        "src/celengine/nebula.h",
        "src/celengine/opencluster.h",
    };

    for (const auto& headerPath : headers)
    {
        const auto header = readSourceFile(headerPath);
        CHECK_FALSE(contains(header, "RenderFlags"));
        CHECK_FALSE(contains(header, "RenderLabels"));
        CHECK_FALSE(contains(header, "getRenderMask("));
        CHECK_FALSE(contains(header, "getLabelMask("));
        CHECK_FALSE(contains(header, "pick("));
        CHECK_FALSE(contains(header, "class Renderer"));
        CHECK_FALSE(contains(header, "meshmanager.h"));
        CHECK_FALSE(contains(header, "GeometryHandle"));
    }
}
