cmake_minimum_required(VERSION 2.8)

project(BKTree_Test)

include(ExternalProject)
find_package(Git REQUIRED)

# LevelDB deps
ExternalProject_Add(
    leveldb
    PREFIX ${CMAKE_BINARY_DIR}/deps
    GIT_REPOSITORY https://github.com/google/leveldb.git
    TIMEOUT 10
    UPDATE_COMMAND ${GIT_EXECUTABLE} pull
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
)

# Test framework
ExternalProject_Add(
    fido
    PREFIX ${CMAKE_BINARY_DIR}/deps
    GIT_REPOSITORY https://github.com/0of/fido.git
    TIMEOUT 10
    UPDATE_COMMAND ${GIT_EXECUTABLE} pull
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(leveldb source_dir)
set(leveldb_src_dir ${source_dir})

ExternalProject_Get_Property(fido source_dir)
set(testframework_dir ${source_dir})

include_directories("${leveldb_src_dir}/include")
include_directories(${testframework_dir})

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../src/")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pthread")
set(CMAKE_BUILD_TYPE Debug)

add_library(libleveldb STATIC IMPORTED)
set_property(TARGET libleveldb PROPERTY
             IMPORTED_LOCATION ${leveldb_src_dir}/out-static/libleveldb.a)

file(GLOB tests "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(BKTree_Test ${tests})
add_dependencies(BKTree_Test leveldb fido)
target_link_libraries(BKTree_Test libleveldb libsnappy ${CMAKE_THREAD_LIBS_INIT})

enable_testing()
add_test(NAME BaseTests COMMAND BKTree_Test)