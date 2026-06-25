#include <celruntime/viewplugin/viewpluginabi.h>

struct CelestiaViewProvider
{
    int value;
};

extern "C" CELESTIA_VIEW_PLUGIN_API int
celestia_view_plugin_abi_version()
{
    return CELESTIA_VIEW_PLUGIN_ABI_VERSION;
}

extern "C" CELESTIA_VIEW_PLUGIN_API const char*
celestia_view_plugin_id()
{
    return "celestia.test.view";
}

extern "C" CELESTIA_VIEW_PLUGIN_API CelestiaViewProvider*
celestia_create_view_provider()
{
    return new CelestiaViewProvider{ 42 };
}

extern "C" CELESTIA_VIEW_PLUGIN_API void
celestia_destroy_view_provider(CelestiaViewProvider* provider)
{
    delete provider;
}
