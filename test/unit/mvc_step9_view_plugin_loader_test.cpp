#include <doctest.h>

#include <filesystem>
#include <string>

#include <celruntime/viewplugin/viewpluginloader.h>

TEST_SUITE_BEGIN("MVC Step9 view plugin loader");

TEST_CASE("ViewPluginLoader loads a test plugin DLL and creates an opaque provider")
{
#ifndef _WIN32
    MESSAGE("Step9 dynamic ViewPluginLoader is Windows-only in the first implementation");
    return;
#else
    std::string error;
    celestia::runtime::viewplugin::ViewPluginLoader loader;

    const auto pluginPath = std::filesystem::path(CELESTIA_TEST_VIEW_PLUGIN_PATH);
    CAPTURE(pluginPath.string());
    REQUIRE(std::filesystem::exists(pluginPath));

    auto plugin = loader.load(pluginPath, &error);
    CAPTURE(error);
    REQUIRE(plugin.has_value());
    CHECK(plugin->id() == "celestia.test.view");
    CHECK(plugin->abiVersion() == CELESTIA_VIEW_PLUGIN_ABI_VERSION);

    auto provider = plugin->createProvider(&error);
    CAPTURE(error);
    CHECK(provider.valid());
#endif
}

TEST_CASE("ViewPluginLoader reports a clear error for a missing plugin")
{
    std::string error;
    celestia::runtime::viewplugin::ViewPluginLoader loader;
    const auto plugin = loader.load(std::filesystem::path("missing-view-plugin.dll"), &error);

    CHECK_FALSE(plugin.has_value());
    CHECK_FALSE(error.empty());
}

TEST_SUITE_END();
