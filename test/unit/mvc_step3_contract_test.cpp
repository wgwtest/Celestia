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

void
checkContains(std::string_view text, std::string_view token)
{
    CAPTURE(token);
    CHECK(contains(text, token));
}

void
checkNoToken(const std::string& source, std::string_view token)
{
    CAPTURE(token);
    CHECK_FALSE(contains(source, token));
}

std::string
sectionBetween(const std::string& text, std::string_view startToken, std::string_view endToken)
{
    const auto start = text.find(startToken);
    CAPTURE(startToken);
    REQUIRE(start != std::string::npos);

    const auto end = text.find(endToken, start);
    CAPTURE(endToken);
    REQUIRE(end != std::string::npos);

    return text.substr(start, end - start);
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step3 physical contract");

TEST_CASE("celengine CMake defines MVC source ownership buckets")
{
    const auto cmake = readSourceFile("src/celengine/CMakeLists.txt");

    checkContains(cmake, "CELESTIA_MODEL_SOURCES");
    checkContains(cmake, "CELESTIA_CONTROLLER_SOURCES");
    checkContains(cmake, "CELESTIA_VIEW_ADAPTER_SOURCES");
    checkContains(cmake, "CELESTIA_VIEW3D_SOURCES");
    checkContains(cmake, "CELESTIA_LEGACY_ENGINE_SOURCES");
}

TEST_CASE("CMake defines physical MVC targets")
{
    const auto celengine = readSourceFile("src/celengine/CMakeLists.txt");
    const auto celestia = readSourceFile("src/celestia/CMakeLists.txt");

    checkContains(celengine, "add_library(celestia_model OBJECT");
    checkContains(celengine, "add_library(celestia_controller OBJECT");
    checkContains(celengine, "add_library(celestia_view_adapter OBJECT");
    checkContains(celengine, "add_library(celestia_view3d OBJECT");

    checkContains(celestia, "$<TARGET_OBJECTS:celestia_model>");
    checkContains(celestia, "$<TARGET_OBJECTS:celestia_controller>");
    checkContains(celestia, "$<TARGET_OBJECTS:celestia_view_adapter>");
    checkContains(celestia, "$<TARGET_OBJECTS:celestia_view3d>");
}

TEST_CASE("CMake source buckets keep model and controller free of view resources")
{
    const auto cmake = readSourceFile("src/celengine/CMakeLists.txt");
    const auto modelSources = sectionBetween(cmake,
                                             "set(CELESTIA_MODEL_SOURCES",
                                             "set(CELESTIA_CONTROLLER_SOURCES");
    const auto controllerSources = sectionBetween(cmake,
                                                  "set(CELESTIA_CONTROLLER_SOURCES",
                                                  "set(CELESTIA_VIEW_ADAPTER_SOURCES");

    constexpr std::string_view forbiddenModelTokens[] = {
        "selectionpicker",
        "bodyrenderassets",
        "starrenderassets",
        "nebularenderassets",
        "render.cpp",
        "render.h",
        "texmanager",
        "meshmanager",
        "shadermanager",
        "dsodbbuilder",
        "stardbbuilder",
        "solarsys.cpp",
        "galaxy.cpp",
        "globular.cpp",
        "marker.cpp",
    };

    constexpr std::string_view forbiddenControllerTokens[] = {
        "selectionpicker",
        "bodyrenderassets",
        "starrenderassets",
        "nebularenderassets",
        "render.cpp",
        "render.h",
        "texmanager",
        "meshmanager",
        "shadermanager",
    };

    for (const auto token : forbiddenModelTokens)
        checkNoToken(modelSources, token);

    for (const auto token : forbiddenControllerTokens)
        checkNoToken(controllerSources, token);
}

TEST_CASE("CMake declares MVC target dependency direction")
{
    const auto cmake = readSourceFile("src/celengine/CMakeLists.txt");

    checkNoToken(cmake, "target_link_libraries(celestia_model");
    checkContains(cmake, "target_link_libraries(celestia_controller PRIVATE celestia_model)");
    checkContains(cmake, "target_link_libraries(celestia_view_adapter PRIVATE celestia_model celestia_controller)");
    checkContains(cmake, "target_link_libraries(celestia_view3d PRIVATE celestia_model celestia_controller celestia_view_adapter)");
}

TEST_CASE("model implementation files do not depend on view adapter or 3D view")
{
    constexpr std::string_view modelFiles[] = {
        "src/celengine/model/body.cpp",
        "src/celengine/model/body.h",
        "src/celengine/model/star.cpp",
        "src/celengine/model/star.h",
        "src/celengine/model/nebula.cpp",
        "src/celengine/model/nebula.h",
        "src/celengine/model/universe.cpp",
        "src/celengine/model/universe.h",
        "src/celengine/model/deepskyobj.cpp",
        "src/celengine/model/deepskyobj.h",
    };

    constexpr std::string_view forbiddenTokens[] = {
        "BodyRenderAssets",
        "StarRenderAssets",
        "NebulaRenderAssets",
        "SelectionPicker",
        "SceneViewModel",
        "Renderer",
        "render.h",
        "meshmanager.h",
        "texmanager.h",
        "shadermanager.h",
    };

    for (const auto file : modelFiles)
    {
        const auto source = readSourceFile(file);
        CAPTURE(file);
        for (const auto token : forbiddenTokens)
            checkNoToken(source, token);
    }
}

TEST_CASE("controller implementation files do not depend on renderer resources")
{
    constexpr std::string_view controllerFiles[] = {
        "src/celengine/controller/simulation.cpp",
        "src/celengine/controller/simulation.h",
        "src/celengine/controller/observer.cpp",
        "src/celengine/controller/observer.h",
        "src/celengine/controller/selection.cpp",
        "src/celengine/controller/selection.h",
    };

    constexpr std::string_view forbiddenTokens[] = {
        "Renderer",
        "GeometryManager",
        "TextureManager",
        "BodyRenderAssets",
        "StarRenderAssets",
        "NebulaRenderAssets",
        "render.h",
        "meshmanager.h",
        "texmanager.h",
        "shadermanager.h",
    };

    for (const auto file : controllerFiles)
    {
        const auto source = readSourceFile(file);
        CAPTURE(file);
        for (const auto token : forbiddenTokens)
            checkNoToken(source, token);
    }
}

TEST_SUITE_END();
