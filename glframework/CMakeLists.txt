file(GLOB_RECURSE FW CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(fw ${FW})
target_include_directories(fw PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
