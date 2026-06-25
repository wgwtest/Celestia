set(CELESTIA_VIEWPROVIDER_3D_SOURCES
  view3dprovider.cpp
  view3dprovider.h
)

add_library(celestia_viewprovider_3d OBJECT ${CELESTIA_VIEWPROVIDER_3D_SOURCES})
target_link_libraries(celestia_viewprovider_3d PRIVATE celestia_runtime celestia_view3d)

set(CELESTIA_VIEWPROVIDER_DEBUG2D_SOURCES
  debug2dprovider.cpp
  debug2dprovider.h
)

add_library(celestia_viewprovider_debug2d OBJECT ${CELESTIA_VIEWPROVIDER_DEBUG2D_SOURCES})
target_link_libraries(celestia_viewprovider_debug2d PRIVATE celestia_runtime)
