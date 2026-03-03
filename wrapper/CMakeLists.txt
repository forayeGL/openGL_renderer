#递归将本文件夹下所有cpp放到FUNCS中
file(GLOB_RECURSE WRAPPER CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

#将FUNCS中所有cpp编译为funcs这个lib库
add_library(wrapper ${WRAPPER})
target_include_directories(wrapper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
