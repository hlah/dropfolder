cmake_minimum_required(VERSION 3.15)
project(Dropfolder)

# Set warning and errors flags
set(warnings "-Wall -Wextra -Wno-error=unused-variable")
set(CMAKE_CXX_FLAGS "${warnings}")
set(CMAKE_C_FLAGS "${warnings}")

# Get source files
file(GLOB_RECURSE COMMON_SOURCE ${CMAKE_SOURCE_DIR}/src/common/*.cpp)
file(GLOB_RECURSE SERVER_SOURCE ${CMAKE_SOURCE_DIR}/src/server/*.cpp)
file(GLOB_RECURSE CLIENT_SOURCE ${CMAKE_SOURCE_DIR}/src/client/*.cpp)

# Get source files
#file(GLOB_RECURSE COMMON_HEADERS ${CMAKE_SOURCE_DIR}/include/common/*.hpp)
#file(GLOB_RECURSE SERVER_HEADERS ${CMAKE_SOURCE_DIR}/include/server/*.hpp)
#file(GLOB_RECURSE CLIENT_HEADERS ${CMAKE_SOURCE_DIR}/include/client/*.hpp)

# Add executables
add_executable(server ${COMMON_SOURCE} ${SERVER_SOURCE})
add_executable(client ${COMMON_SOURCE} ${CLIENT_SOURCE})

# Include folders
include_directories(${CMAKE_SOURCE_DIR}/include/common)
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/include/client)
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/include/server)

# Tests
add_executable(watcher_test ${CMAKE_SOURCE_DIR}/tests/src/watcher_test.cpp ${COMMON_SOURCE})
