cmake_minimum_required(VERSION 3.13)

if(DEFINED ENV{VITASDK} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain")
endif()

project(padsounds C)
set(CMAKE_C_STANDARD 11)

if(VITA)
  # --- PS Vita (VitaSDK) ---
  include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
  set(VITA_APP_NAME "Pad Sounds")
  set(VITA_TITLEID "PADS00001")
  set(VITA_VERSION "01.00")

  add_executable(padsounds main.c)
  target_include_directories(padsounds PRIVATE "$ENV{VITASDK}/arm-vita-eabi/include/SDL2")
  target_link_libraries(padsounds
    SDL2_mixer SDL2 vorbisfile vorbis ogg mpg123 FLAC m
    SceAudio_stub SceAudioIn_stub SceCtrl_stub SceDisplay_stub SceGxm_stub
    SceHid_stub SceMotion_stub SceTouch_stub SceCommonDialog_stub SceSysmodule_stub)

  vita_create_self(padsounds.self padsounds UNSAFE)

  # Los .wav de sounds/ van dentro del VPK (se leen de app0:sounds/)
  file(GLOB SOUND_FILES RELATIVE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/sounds/*.wav)
  set(VPK_FILES "")
  foreach(f ${SOUND_FILES})
    list(APPEND VPK_FILES FILE ${CMAKE_SOURCE_DIR}/${f} ${f})
  endforeach()

  vita_create_vpk(padsounds.vpk ${VITA_TITLEID} padsounds.self
    VERSION ${VITA_VERSION} NAME ${VITA_APP_NAME}
    ${VPK_FILES})
else()
  # --- PC (para probar rapido) ---
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED sdl2 SDL2_mixer)

  add_executable(padsounds main.c)
  target_include_directories(padsounds PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(padsounds ${SDL2_LIBRARIES})
endif()
