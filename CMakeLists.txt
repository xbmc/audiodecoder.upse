project(audiodecoder.upse)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(xbmc REQUIRED)

include_directories(${XBMC_INCLUDE_DIR}
                    ${PROJECT_SOURCE_DIR}/lib/libupse)

add_subdirectory(lib/libupse)

set(UPSE_SOURCES src/UPSECodec.cpp)

set(DEPLIBS -Wl,--whole-archive upse -Wl,--no-whole-archive)

build_addon(audiodecoder.upse UPSE DEPLIBS)

include(CPack)
