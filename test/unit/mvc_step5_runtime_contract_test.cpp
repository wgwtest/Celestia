#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

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

class FakeViewRuntime final : public celestia::runtime::ViewRuntime
{
public:
    explicit FakeViewRuntime(std::string id) : m_id(std::move(id)) {}

    std::string_view id() const override
    {
        return m_id;
    }

private:
    std::string m_id;
};

celestia::runtime::ViewProvider
makeFakeProvider(std::string id, std::string dimension)
{
    celestia::runtime::ViewProvider provider;
    provider.id = std::move(id);
    provider.displayName = provider.id;
    provider.dimension = std::move(dimension);
    provider.create = [id = provider.id]()
    {
        return std::make_unique<FakeViewRuntime>(id);
    };
    return provider;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step5 runtime contract");

TEST_CASE("runtime contract target is renderer-free")
{
    CHECK(std::filesystem::exists(repoPath("src/celruntime/CMakeLists.txt")));

    const auto runtimeCmake = readSourceFile("src/celruntime/CMakeLists.txt");
    CHECK(contains(runtimeCmake, "add_library(celestia_runtime OBJECT"));
    CHECK_FALSE(contains(runtimeCmake, "view3d/render.h"));
    CHECK_FALSE(contains(runtimeCmake, "render.cpp"));
    CHECK_FALSE(contains(runtimeCmake, "meshmanager"));
    CHECK_FALSE(contains(runtimeCmake, "texmanager"));

    const auto appCmake = readSourceFile("src/celestia/CMakeLists.txt");
    CHECK(contains(appCmake, "$<TARGET_OBJECTS:celestia_runtime>"));
}

TEST_CASE("view provider registry registers and selects providers by id")
{
    celestia::runtime::ViewProviderRegistry registry;
    CHECK(registry.empty());

    CHECK(registry.registerProvider(makeFakeProvider("celestia.view3d.opengl", "3d")));
    CHECK(registry.registerProvider(makeFakeProvider("celestia.view2d.debug", "2d")));
    CHECK_FALSE(registry.registerProvider(makeFakeProvider("celestia.view2d.debug", "2d")));

    CHECK(registry.size() == 2);

    const auto* view3d = registry.find("celestia.view3d.opengl");
    REQUIRE(view3d != nullptr);
    CHECK(view3d->dimension == "3d");

    const auto* view2d = registry.find("celestia.view2d.debug");
    REQUIRE(view2d != nullptr);
    CHECK(view2d->dimension == "2d");

    auto runtime = registry.create("celestia.view2d.debug");
    REQUIRE(runtime != nullptr);
    CHECK(runtime->id() == "celestia.view2d.debug");

    CHECK(registry.create("missing.view") == nullptr);
}

TEST_SUITE_END();
