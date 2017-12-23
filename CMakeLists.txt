cmake_minimum_required(VERSION 3.9)
project(ast-variants CXX)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(include)

if(MSVC)
    add_compile_options(/std:c++17 /W4 /wd4100 /wd4456 /permissive- /GR-)
else()
    add_compile_options(
        -std=c++1z
        -Wall
        -Wextra
        -pedantic
    )
endif()

find_package(fmt REQUIRED)
find_package(Boost 1.62.0 COMPONENTS program_options)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(bat src/bat.cpp)
    target_link_libraries(bat ${Boost_LIBRARIES})
endif()

target_link_libraries(bat ${Boost_LIBRARIES} fmt::fmt-header-only)
