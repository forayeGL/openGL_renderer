#需求的最低cmake程序版本
cmake_minimum_required(VERSION 3.12)
add_definitions (-DDEBUG)

#本工程的名字
project(OpenGLRenderer)

#本工程支持的C++版本
set(CMAKE_CXX_STANDARD 17)


file(GLOB ASSETS "./assets" )

file(COPY ${ASSETS} DESTINATION ${CMAKE_BINARY_DIR})


include_directories(
	SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/thirdParty/include
	SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/application/miniz
)

link_directories(
	SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/thirdParty/lib
)

add_subdirectory(wrapper)
add_subdirectory(application)
add_subdirectory(glframework)
add_subdirectory(imgui)

#本工程所有cpp文件编译链接，生成exe
add_executable(openglRenderer "main.cpp" "glad.c" )

target_link_libraries(openglRenderer glfw3.lib  assimp-vc143-mtd.lib zlibstaticd.lib   wrapper app fw imguilib)

# IBL baking tool (standalone executable, no imgui/app dependency)
add_executable(iblBaker "tools/iblBaker/main.cpp" "glad.c")
target_include_directories(iblBaker PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(iblBaker glfw3.lib zlibstaticd.lib wrapper app fw)