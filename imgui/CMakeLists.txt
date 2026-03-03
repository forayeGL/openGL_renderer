file(GLOB_RECURSE IMGUI CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(imguilib ${IMGUI})
target_include_directories(imguilib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
