cmake_minimum_required(VERSION 3.5)
project(Dropfolder)
# compile flags for editor completion
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

# Set warning and errors flags
set(warnings "-Wall -Wextra -Wno-error=unused-variable")
set(CMAKE_CXX_FLAGS "-g ${warnings}")
set(CMAKE_C_FLAGS "-g ${warnings}")
set(CMAKE_CXX_STANDARD 11)

# Find pthreads
find_package(Threads)

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
target_link_libraries(client ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(server ${CMAKE_THREAD_LIBS_INIT})
message(STATUS ${CMAKE_THREAD_LIBS_INIT})


# Include folders
include_directories(${CMAKE_SOURCE_DIR}/include/common)
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/include/client)
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/include/server)

# Tests
add_executable(watcher_test ${CMAKE_SOURCE_DIR}/tests/src/watcher_test.cpp ${COMMON_SOURCE})
add_executable(client_test ${CMAKE_SOURCE_DIR}/tests/src/client_test.cpp ${COMMON_SOURCE})
add_executable(server_test ${CMAKE_SOURCE_DIR}/tests/src/server_test.cpp ${COMMON_SOURCE})
add_executable(change_port_test ${CMAKE_SOURCE_DIR}/tests/src/change_port_test.cpp ${COMMON_SOURCE})
add_executable(connection_info ${CMAKE_SOURCE_DIR}/tests/src/connection_info.cpp ${COMMON_SOURCE})
add_executable(barrier_test ${CMAKE_SOURCE_DIR}/tests/src/barrier_test.cpp ${COMMON_SOURCE})
target_link_libraries(watcher_test ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(server_test ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(client_test ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(change_port_test ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(connection_info ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(barrier_test ${CMAKE_THREAD_LIBS_INIT})
