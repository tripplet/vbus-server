CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
include(ExternalProject)

PROJECT(vbus-server)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_STANDARD 11)
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
include_directories(uriparser/include/)

add_subdirectory(SQLiteCpp)

ExternalProject_Add(liburiparser
  PREFIX ${CMAKE_SOURCE_DIR}/uriparser
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/uriparser
  BINARY_DIR ${CMAKE_SOURCE_DIR}/uriparser
  CONFIGURE_COMMAND ./autogen.sh && ./configure --disable-test --disable-doc
  BUILD_COMMAND make
  INSTALL_COMMAND ""
)

# Add uriparser library (ensure static version is linked)
ADD_LIBRARY(uriparser STATIC IMPORTED)
SET_TARGET_PROPERTIES(uriparser PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/uriparser/.libs/liburiparser.a)

# Add src code directory
add_subdirectory(src)

add_executable(vbus-server ${SOURCES})

target_link_libraries(vbus-server SQLiteCpp sqlite3 uriparser)
add_dependencies(vbus-server SQLiteCpp)
