cmake_minimum_required(VERSION 3.16)
project(UnitTest VERSION 3.1.2)

set(CMAKE_CXX_STANDARD 20)

include_directories(${LIB_PATH}/Yarns/includes)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -pthread -fPIC")

add_executable(UnitTest test.cpp Includes/catch.hpp)

IF (WIN32)
    set(NAME Yarns)
    set(EXT lib)
else()
    set(NAME libYarns)
    set(EXT a)
endif()

target_link_libraries(UnitTest PUBLIC ${LIB_DIR}/${NAME}.${EXT})