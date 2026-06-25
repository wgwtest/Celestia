set(CELESTIA_RUNTIME_SOURCES
  runtimeconfig.cpp
  runtimeconfig.h
  viewprovider.h
  viewproviderregistry.cpp
  viewproviderregistry.h
  viewruntime.h
)

add_library(celestia_runtime OBJECT ${CELESTIA_RUNTIME_SOURCES})
