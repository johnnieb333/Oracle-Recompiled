cmake_minimum_required(VERSION 3.16)
project(OracleSourcePreview C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-O3)
endif()
find_package(SDL2 REQUIRED)
set(GBRT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/runtime")
add_library(gbrt STATIC
  ${GBRT_DIR}/src/gbrt.c ${GBRT_DIR}/src/ppu.c ${GBRT_DIR}/src/audio.c
  ${GBRT_DIR}/src/interpreter.c ${GBRT_DIR}/src/platform_sdl.cpp
  ${GBRT_DIR}/src/hwtrace.c ${GBRT_DIR}/src/menu_gui.cpp ${GBRT_DIR}/src/asset_viewer.cpp)
foreach(name imgui imgui_draw imgui_tables imgui_widgets imgui_demo)
  target_sources(gbrt PRIVATE ${GBRT_DIR}/third_party/imgui/${name}.cpp)
endforeach()
target_sources(gbrt PRIVATE
  ${GBRT_DIR}/third_party/imgui/backends/imgui_impl_sdl2.cpp
  ${GBRT_DIR}/third_party/imgui/backends/imgui_impl_sdlrenderer2.cpp)
target_include_directories(gbrt PUBLIC ${GBRT_DIR}/include ${GBRT_DIR}/src ${GBRT_DIR}/third_party/imgui)
target_compile_definitions(gbrt PUBLIC GB_HAS_SDL2 LA_HAS_IMGUI)
target_link_libraries(gbrt PUBLIC SDL2::SDL2)
foreach(game ages seasons)
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/generated/${game}/game.c")
    message(FATAL_ERROR "Generate both games with tools/prepare_game.py before configuring.")
  endif()
  add_executable(oracle-${game} src/oracle_main.c src/oracle_rom.c generated/${game}/game.c)
  target_link_libraries(oracle-${game} PRIVATE gbrt)
  target_include_directories(oracle-${game} PRIVATE generated/${game})
  set_target_properties(oracle-${game} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${game}")
endforeach()
target_compile_definitions(oracle-ages PRIVATE
  ORACLE_ROM_SHA1="880374fb978b18af4aa529e2e32f7ffb4d7dd2f4"
  ORACLE_GAME_TITLE="Oracle of Ages - External ROM Test")
target_compile_definitions(oracle-seasons PRIVATE
  ORACLE_ROM_SHA1="ba1268290fb2b1b70505d2d7b5825fc8a4816a4b"
  ORACLE_GAME_TITLE="Oracle of Seasons - External ROM Test")
