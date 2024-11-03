# Minimum CMake version
cmake_minimum_required(VERSION 3.20)

# Project name and settings
project(NoExceptionProject VERSION 1.0 LANGUAGES CXX)

# Set C++ standard and disable exceptions
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Boost libraries
find_package(Boost REQUIRED COMPONENTS cobalt)


# Glob for all source files in the src directory
file(GLOB SOURCES "src/*.cpp")

# Create an executable target
add_executable(NoExceptionProject ${SOURCES})

# Link against Boost libraries
target_link_libraries(NoExceptionProject PRIVATE ${Boost_LIBRARIES})

# Include directory for Boost headers
target_include_directories(NoExceptionProject PRIVATE ${Boost_INCLUDE_DIRS} include)
