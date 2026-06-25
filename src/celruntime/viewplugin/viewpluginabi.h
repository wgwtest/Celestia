// viewpluginabi.h
//
// Copyright (C) 2026, the Celestia Development Team

#pragma once

#define CELESTIA_VIEW_PLUGIN_ABI_VERSION 1

#if defined(_WIN32)
#if defined(CELESTIA_VIEW_PLUGIN_BUILD)
#define CELESTIA_VIEW_PLUGIN_API __declspec(dllexport)
#else
#define CELESTIA_VIEW_PLUGIN_API __declspec(dllimport)
#endif
#else
#define CELESTIA_VIEW_PLUGIN_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CelestiaViewProvider CelestiaViewProvider;

typedef int (*CelestiaViewPluginAbiVersionFn)(void);
typedef const char* (*CelestiaViewPluginIdFn)(void);
typedef CelestiaViewProvider* (*CelestiaCreateViewProviderFn)(void);
typedef void (*CelestiaDestroyViewProviderFn)(CelestiaViewProvider*);

CELESTIA_VIEW_PLUGIN_API int celestia_view_plugin_abi_version(void);
CELESTIA_VIEW_PLUGIN_API const char* celestia_view_plugin_id(void);
CELESTIA_VIEW_PLUGIN_API CelestiaViewProvider* celestia_create_view_provider(void);
CELESTIA_VIEW_PLUGIN_API void celestia_destroy_view_provider(CelestiaViewProvider*);

#ifdef __cplusplus
}
#endif
