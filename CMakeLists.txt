cmake_minimum_required(VERSION 3.7)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(liblist boost_program_options ON)
set(liblist boost_optional ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost 1.62.0 COMPONENTS program_options)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(bat src/bat.cpp)
    target_link_libraries(bat ${Boost_LIBRARIES})
endif()

target_link_libraries(bat ${Boost_LIBRARIES})
