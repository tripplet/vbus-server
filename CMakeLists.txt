CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

PROJECT(vbus-server)

# Define the two required variables before including
# the source code for watching a git repository.
set(PRE_CONFIGURE_FILE "src/gitversion.cpp.in")
set(POST_CONFIGURE_FILE "src/gitversion.cpp")
include(git_watcher.cmake)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto")

IF (DEFINED DB_PATH)
    ADD_COMPILE_DEFINITIONS(DB_PATH="${DB_PATH}")
ENDIF()

IF (DEFINED BROTLI_SUPPORT)
    ADD_COMPILE_DEFINITIONS(BROTLI_SUPPORT=${BROTLI_SUPPORT})
ENDIF()

# Put result binary in topmost directory
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Disable some unused SQLiteCpp targets
set(SQLITECPP_RUN_DOXYGEN OFF CACHE BOOL "")
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "")
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "")

# Brotli lib settings
set(BROTLI_BUNDLED_MODE ON)
set(BROTLI_DISABLE_TESTS)

# uriparser lib settings
SET(BUILD_SHARED_LIBS OFF CACHE BOOL "")
SET(URIPARSER_BUILD_DOCS OFF CACHE BOOL "")
SET(URIPARSER_BUILD_TESTS OFF CACHE BOOL "")
SET(URIPARSER_BUILD_TOOLS OFF CACHE BOOL "")

# Add libraries
include_directories(SQLiteCpp/include/)
include_directories(uriparser/include/)

add_subdirectory(SQLiteCpp)
add_subdirectory(uriparser)

IF (DEFINED BROTLI_SUPPORT AND BROTLI_SUPPORT EQUAL 1)
  include_directories(brotli/c/include/)
  add_subdirectory(brotli)
  set(BROTLI_TARGET_LINK brotlienc)
  set(BROTLI_DEPENDENCY brotli)
ENDIF()

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

# Add src code directory
add_subdirectory(src)

add_executable(vbus-server ${SOURCES})
target_link_libraries(vbus-server SQLiteCpp sqlite3 uriparser ${BROTLI_TARGET_LINK} z Threads::Threads ${CMAKE_DL_LIBS})
add_dependencies(vbus-server SQLiteCpp ${BROTLI_DEPENDENCY} uriparser)

# Copy output binary to web/vbus-server.cgi
add_custom_command(TARGET vbus-server POST_BUILD COMMAND cp "$<TARGET_FILE:vbus-server>" "${CMAKE_SOURCE_DIR}/web/vbus-server.cgi")
