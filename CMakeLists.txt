
cmake_minimum_required(VERSION 3.0)

project(BKTree)

include(ExternalProject)
find_package(Git REQUIRED)

ExternalProject_Add(
    leveldb_src
    PREFIX ${CMAKE_BINARY_DIR}/deps
    GIT_REPOSITORY https://github.com/google/leveldb.git
    TIMEOUT 10
    UPDATE_COMMAND ${GIT_EXECUTABLE} pull
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(leveldb_src source_dir)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
set(CMAKE_BUILD_TYPE Debug)

include_directories(${source_dir}/include/)
file(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(leveldb STATIC IMPORTED)
set_property(TARGET leveldb PROPERTY
             IMPORTED_LOCATION ${source_dir}/out-static/libleveldb.a)

add_executable(BKTree ${files})
target_link_libraries(BKTree leveldb ${CMAKE_THREAD_LIBS_INIT})
