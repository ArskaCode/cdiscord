cmake_minimum_required(VERSION 3.17)
project(cdiscord C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_MODULE_PATH ${cdiscord_SOURCE_DIR}/cmake/)

find_package(libwebsockets REQUIRED)
find_package(json-c REQUIRED)
find_package(CURL REQUIRED)

add_subdirectory(src)
