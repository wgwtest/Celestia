#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <celruntime/viewplugin/viewpluginabi.h>
#include <celruntime/viewplugin/viewpluginlifecycle.h>

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
    std::ifstream input(sourceRoot() / std::filesystem::path(relativePath));
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

TEST_SUITE_BEGIN("MVC Step9 view plugin ABI");

TEST_CASE("View plugin ABI exposes stable C-compatible entry points")
{
    CHECK(CELESTIA_VIEW_PLUGIN_ABI_VERSION == 1);

    const auto header = readSourceFile("src/celruntime/viewplugin/viewpluginabi.h");
    CHECK(contains(header, "extern \"C\""));
    CHECK(contains(header, "celestia_view_plugin_abi_version"));
    CHECK(contains(header, "celestia_view_plugin_id"));
    CHECK(contains(header, "celestia_create_view_provider"));
    CHECK(contains(header, "celestia_destroy_view_provider"));
    CHECK_FALSE(contains(header, "std::"));
}

TEST_CASE("View plugin lifecycle permits ordered start stop shutdown transitions")
{
    using celestia::runtime::viewplugin::ViewPluginState;
    using celestia::runtime::viewplugin::canTransition;

    CHECK(canTransition(ViewPluginState::Loaded, ViewPluginState::Configured));
    CHECK(canTransition(ViewPluginState::Configured, ViewPluginState::Started));
    CHECK(canTransition(ViewPluginState::Started, ViewPluginState::Stopping));
    CHECK(canTransition(ViewPluginState::Stopping, ViewPluginState::Stopped));
    CHECK(canTransition(ViewPluginState::Stopped, ViewPluginState::Shutdown));
    CHECK(canTransition(ViewPluginState::Started, ViewPluginState::Failed));
    CHECK_FALSE(canTransition(ViewPluginState::Started, ViewPluginState::Loaded));
}

TEST_SUITE_END();
