# Project init
cmake_minimum_required(VERSION 3.16)
project(vulkan-compute)
set (CMAKE_CXX_STANDARD 17)

# Source files
file(GLOB_RECURSE SOURCE_FILES 
${CMAKE_SOURCE_DIR}/src/*.c
${CMAKE_SOURCE_DIR}/src/*.cpp)

# Header files
file(GLOB_RECURSE HEADER_FILES 
${CMAKE_SOURCE_DIR}/src/*.h
${CMAKE_SOURCE_DIR}/src/*.hpp)

# Executable
add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

# Vulkan
find_package (Vulkan REQUIRED)
include_directories({$Vulkan_INCLUDE_DIRS})

# GLFW
add_subdirectory(external/glfw)

# Linker refs
include_directories(
  PUBLIC external/glfw/include
  PUBLIC external/glm
  PUBLIC external/stb)

target_link_directories(${PROJECT_NAME} PRIVATE external/glfw/src)

set(LIBS Vulkan::Vulkan glfw)

target_link_libraries(${PROJECT_NAME} ${LIBS})