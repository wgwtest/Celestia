set(CELESTIA_RUNTIME_SOURCES
  assembly/runtimeassembly.cpp
  assembly/runtimeassembly.h
  assembly/runtimeassemblyconfig.cpp
  assembly/runtimeassemblyconfig.h
  assembly/runtimeassemblyrunner.cpp
  assembly/runtimeassemblyrunner.h
  controller/controllerservice.cpp
  controller/controllerservice.h
  controllerruntime.cpp
  controllerruntime.h
  dataplane/dataplanechannel.h
  dataplane/dataplaneref.cpp
  dataplane/dataplaneref.h
  dataplane/inprocessdataplane.cpp
  dataplane/inprocessdataplane.h
  dataplane/sharedmemorydataplane.cpp
  dataplane/sharedmemorydataplane.h
  ipc/channel.h
  ipc/localchannel.cpp
  ipc/localchannel.h
  ipc/message.cpp
  ipc/message.h
  model/modelservice.cpp
  model/modelservice.h
  model/modelsnapshot.h
  model/sceneextractor.cpp
  model/sceneextractor.h
  modelruntime.cpp
  modelruntime.h
  protocol/envelope.cpp
  protocol/envelope.h
  protocol/lifecycle.h
  protocol/payloadtype.h
  protocol/sceneprotocol.cpp
  protocol/sceneprotocol.h
  protocol/viewinput.cpp
  protocol/viewinput.h
  process/hostprocess.cpp
  process/hostprocess.h
  process/hostprocess_win32.cpp
  process/processsupervisor.cpp
  process/processsupervisor.h
  process/runtimesession.cpp
  process/runtimesession.h
  runtimeconfig.cpp
  runtimeconfig.h
  transport/framedmessage.cpp
  transport/framedmessage.h
  transport/framedtransport.h
  transport/localsockettransport.cpp
  transport/localsockettransport.h
  transport/runtimetransport.h
  transport/stdiopipetransport.cpp
  transport/stdiopipetransport.h
  transport/stdiotransport.cpp
  transport/stdiotransport.h
  transport/transportfactory.cpp
  transport/transportfactory.h
  runtimecomposition.cpp
  runtimecomposition.h
  view3d/view3dhost.cpp
  view3d/view3dhost.h
  viewplugin/viewpluginlifecycle.cpp
  viewplugin/viewpluginlifecycle.h
  viewplugin/viewpluginloader.cpp
  viewplugin/viewpluginloader.h
  viewplugin/viewpluginmanifest.cpp
  viewplugin/viewpluginmanifest.h
  viewplugin/viewpluginregistry.cpp
  viewplugin/viewpluginregistry.h
  viewadapterruntime.cpp
  viewadapterruntime.h
  view/viewservice.cpp
  view/viewservice.h
  viewframecodec.cpp
  viewframecodec.h
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
  process/runtimehostloop.cpp
  process/runtimehostloop.h
  process/view3dhostloop.cpp
  process/view3dhostloop.h
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

add_executable(celestia-view3d-host
  process/view3dhostmain.cpp
  ${CELESTIA_RUNTIME_HOST_COMMON_SOURCES}
  $<TARGET_OBJECTS:celestia_runtime>
)

if(ENABLE_SDL)
  find_package(SDL2 CONFIG REQUIRED)
  target_sources(celestia-view3d-host PRIVATE
    view3d/view3dloop.cpp
    view3d/view3dloop.h
  )
  target_compile_definitions(celestia-view3d-host PRIVATE CELESTIA_RUNTIME_VIEW3D_SDL)
  target_link_libraries(celestia-view3d-host PRIVATE SDL2::SDL2main SDL2::SDL2)
  if(TARGET OpenGL::GL)
    target_link_libraries(celestia-view3d-host PRIVATE OpenGL::GL)
  endif()
endif()
