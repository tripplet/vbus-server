CMAKE_MINIMUM_REQUIRED(VERSION 2.8.5)
PROJECT(vbus-server)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Add jsoncpp
#include_directories(jsoncpp/dist/)
#set(SOURCES jsoncpp/dist/jsoncpp.cpp)

# Disable some unused SQLiteCpp targets
SET(SQLITECPP_RUN_DOXYGEN OFF CACHE BOOL "Run Doxygen C++ documentation tool.")
SET(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "Run cppcheck C++ static analysis tool.")
SET(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "Run cpplint.py tool for Google C++ StyleGuide.")

# Add SQLiteCpp
include_directories(SQLiteCpp/include/)
add_subdirectory(SQLiteCpp)

# Add src code directory
add_subdirectory(src)

add_executable(vbus-server ${SOURCES})

target_link_libraries(vbus-server SQLiteCpp sqlite3)
add_dependencies(vbus-server SQLiteCpp)
