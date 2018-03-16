project(audiodecoder.upse)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(${KODI_INCLUDE_DIR}/..
                    ${PROJECT_SOURCE_DIR}/lib/libupse)

add_subdirectory(lib/libupse)

set(UPSE_SOURCES src/UPSECodec.cpp)

if(UNIX AND NOT APPLE)
  set(DEPLIBS -Wl,--whole-archive upse -Wl,--no-whole-archive ${ZLIB_LIBRARIES})
else()
  set(DEPLIBS upse ${ZLIB_LIBRARIES})
endif()

build_addon(audiodecoder.upse UPSE DEPLIBS)

include(CPack)
