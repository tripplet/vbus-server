CMAKE_MINIMUM_REQUIRED(VERSION 2.8.5)
PROJECT(vbus-server)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Add jsoncpp
include_directories(jsoncpp/dist/)
set(SOURCES jsoncpp/dist/jsoncpp.cpp)

# Add SQLiteCpp
include_directories(SQLiteCpp/include/)
add_subdirectory(SQLiteCpp)

# Add src code directory
add_subdirectory(src)

add_executable(vbus-server ${SOURCES})

target_link_libraries(vbus-server sqlite3 SQLiteCpp)
add_dependencies(vbus-server SQLiteCpp)