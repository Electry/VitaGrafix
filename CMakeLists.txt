cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(VitaGrafix)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

if (NOT ${RELEASE})
  add_definitions(-DENABLE_LOGGING)
endif()

add_executable(VitaGrafix
  main.c
  display.c
  config.c
  tools.c
  games.c
)

target_link_libraries(VitaGrafix
  taihen_stub
  SceAppMgr_stub
  SceDisplay_stub
  SceCtrl_stub
  kuio_stub
  k
  gcc
)

set_target_properties(VitaGrafix
  PROPERTIES LINK_FLAGS "-nostdlib"
)

vita_create_self(VitaGrafix.suprx VitaGrafix
  CONFIG ${CMAKE_SOURCE_DIR}/VitaGrafix.yml
)
