cmake_minimum_required(VERSION 3.30)
project(sync)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CURL_ROOT "C:/libs/curl82")

add_library(curl SHARED IMPORTED)
set_target_properties(curl PROPERTIES
        IMPORTED_LOCATION "${CURL_ROOT}/bin/libcurl-x64.dll"
        IMPORTED_IMPLIB "${CURL_ROOT}/lib/libcurl.dll.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CURL_ROOT}/include"
)
add_library(connect STATIC src/connect.cpp)
target_include_directories(connect PUBLIC include "${CURL_ROOT}/include")
target_link_directories(connect PRIVATE curl)


add_library(sync STATIC src/library.cpp)
target_include_directories(sync PUBLIC include "${CURL_ROOT}/include")
target_link_directories(sync PRIVATE connect)

add_executable(sync_test test/main.cpp)
target_link_libraries(sync_test PRIVATE sync curl)
