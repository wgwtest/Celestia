#include <doctest.h>

#include <string>

#include <celruntime/viewplugin/viewpluginmanifest.h>

namespace
{

constexpr const char* ValidManifest = R"json(
{
  "id": "celestia.view3d.opengl",
  "name": "Celestia OpenGL 3D View",
  "kind": "view3d",
  "version": "0.1.0",
  "protocolVersion": 1,
  "entry": "celestia-view3d-plugin.dll",
  "processMode": "host-process",
  "capabilities": ["scene.frame", "view.input", "opengl", "sdl-window"],
  "supportedTransports": ["stdio-pipe"],
  "resourceRequirements": ["content-root"]
}
)json";

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step9 view plugin manifest");

TEST_CASE("View plugin manifest parses identity, entry, protocol and capabilities")
{
    std::string error;
    const auto manifest = celestia::runtime::viewplugin::parseViewPluginManifest(ValidManifest, &error);

    CAPTURE(error);
    REQUIRE(manifest.has_value());
    CHECK(manifest->id == "celestia.view3d.opengl");
    CHECK(manifest->name == "Celestia OpenGL 3D View");
    CHECK(manifest->kind == "view3d");
    CHECK(manifest->protocolVersion == celestia::runtime::viewplugin::CurrentViewPluginProtocolVersion);
    CHECK(manifest->entry == "celestia-view3d-plugin.dll");
    CHECK(manifest->processMode == "host-process");
    CHECK(manifest->supports("scene.frame"));
    CHECK(manifest->supports("view.input"));
    CHECK(manifest->supports("opengl"));
    CHECK_FALSE(manifest->supports("metal"));
}

TEST_CASE("View plugin manifest rejects missing id")
{
    std::string error;
    const auto manifest = celestia::runtime::viewplugin::parseViewPluginManifest(
        R"json({"name":"Broken","kind":"view3d","version":"0.1.0","protocolVersion":1,"entry":"x.dll"})json",
        &error);

    CHECK_FALSE(manifest.has_value());
    CHECK(error.find("id") != std::string::npos);
}

TEST_CASE("View plugin manifest rejects incompatible protocol version")
{
    std::string error;
    const auto manifest = celestia::runtime::viewplugin::parseViewPluginManifest(
        R"json({"id":"broken","name":"Broken","kind":"view3d","version":"0.1.0","protocolVersion":99,"entry":"x.dll"})json",
        &error);

    CHECK_FALSE(manifest.has_value());
    CHECK(error.find("protocolVersion") != std::string::npos);
}

TEST_SUITE_END();
