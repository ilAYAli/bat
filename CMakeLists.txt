cmake_minimum_required(VERSION 3.7)

# debug:
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# CXX:
set(CMAKE_CXX_COMPILER=clang++)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wunused")

include_directories(.)
include_directories(src)

# /bin/main
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#add_executable(bat src/bat.cpp)

find_package(Boost 1.62.0 COMPONENTS program_options)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(bat src/bat.cpp)
    target_link_libraries(bat ${Boost_LIBRARIES})
endif()

target_link_libraries(bat ${Boost_LIBRARIES})
