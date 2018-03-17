cmake_minimum_required(VERSION 3.7)

project(bat)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")

include_directories(include)

# libfmt:
find_package(fmt REQUIRED)
if (NOT fmt_FOUND)
    message (FATAL_ERROR "Cannot find libfmt")
endif()

# boost:
find_package(Boost 1.62.0 REQUIRED COMPONENTS program_options)
if (NOT Boost_FOUND)
    message (FATAL_ERROR "Cannot find Boost")
else()
    include_directories(${Boost_INCLUDE_DIRS})
endif()

# target:
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(bat src/main.cpp)

# link:
target_link_libraries(bat ${Boost_LIBRARIES} fmt)
