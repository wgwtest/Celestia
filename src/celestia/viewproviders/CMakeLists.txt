set(CELESTIA_VIEWPROVIDER_SOURCES
  view3dprovider.cpp
  view3dprovider.h
)

add_library(celestia_viewproviders OBJECT ${CELESTIA_VIEWPROVIDER_SOURCES})
target_link_libraries(celestia_viewproviders PRIVATE celestia_runtime celestia_view3d)
