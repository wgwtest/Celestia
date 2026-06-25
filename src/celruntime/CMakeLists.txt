set(CELESTIA_RUNTIME_SOURCES
  controllerruntime.cpp
  controllerruntime.h
  modelruntime.cpp
  modelruntime.h
  runtimeconfig.cpp
  runtimeconfig.h
  runtimecomposition.cpp
  runtimecomposition.h
  viewadapterruntime.cpp
  viewadapterruntime.h
  viewframe.h
  viewprovider.h
  viewproviderregistry.cpp
  viewproviderregistry.h
  viewruntime.h
)

add_library(celestia_runtime OBJECT ${CELESTIA_RUNTIME_SOURCES})
