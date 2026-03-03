#递归将本文件夹下所有cpp
file(GLOB_RECURSE APP CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_library(app ${APP})
target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
