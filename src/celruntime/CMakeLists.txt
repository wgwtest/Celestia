set(CELESTIA_RUNTIME_SOURCES
  controllerruntime.cpp
  controllerruntime.h
  ipc/channel.h
  ipc/localchannel.cpp
  ipc/localchannel.h
  ipc/message.cpp
  ipc/message.h
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

set(CELESTIA_RUNTIME_HOST_COMMON_SOURCES
  process/runtimehostcommon.cpp
  process/runtimehostcommon.h
)

add_executable(celestia-model-host
  process/modelhostmain.cpp
  ${CELESTIA_RUNTIME_HOST_COMMON_SOURCES}
  $<TARGET_OBJECTS:celestia_runtime>
)

add_executable(celestia-controller-host
  process/controllerhostmain.cpp
  ${CELESTIA_RUNTIME_HOST_COMMON_SOURCES}
  $<TARGET_OBJECTS:celestia_runtime>
)

add_executable(celestia-view-host
  process/viewhostmain.cpp
  ${CELESTIA_RUNTIME_HOST_COMMON_SOURCES}
  $<TARGET_OBJECTS:celestia_runtime>
)
