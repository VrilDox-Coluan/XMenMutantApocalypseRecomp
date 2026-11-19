cmake_minimum_required(VERSION 3.20)
project(XMenMutantApocalypseRecomp C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(SNESRECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/snesrecomp" CACHE PATH
    "Path to snesrecomp framework")
set(XMEN_GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated" CACHE PATH
    "Directory containing generated snesrecomp C sources")

# OBJECT library retains the generated module-registration constructor in module_v2.c.
add_library(snesrecomp_game OBJECT)
target_include_directories(snesrecomp_game PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/config"
    "${SNESRECOMP_ROOT}/runner/src"
    "${SNESRECOMP_ROOT}/runner/src/snes")
if(NOT MSVC)
    target_compile_options(snesrecomp_game PRIVATE
        -w -Wno-implicit-function-declaration)
endif()

include("${SNESRECOMP_ROOT}/runner/runner.cmake")
snesrecomp_target_generated_code(snesrecomp_game "${XMEN_GEN_DIR}")

set(XMEN_RUNNER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/runner")
add_executable(xmen_headless
    ${SNESRECOMP_RUNNER_SOURCES}
    ${XMEN_RUNNER_DIR}/wotg_game.c
    ${XMEN_RUNNER_DIR}/headless_host.c
    ${XMEN_RUNNER_DIR}/host_contract.c
    ${XMEN_RUNNER_DIR}/headless_main.c)
target_include_directories(xmen_headless PRIVATE
    ${SNESRECOMP_RUNNER_INCLUDE_DIRS}
    ${SNESRECOMP_ROOT}/runner/src/desktop
    ${XMEN_RUNNER_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/config")
target_link_libraries(xmen_headless PRIVATE
    snesrecomp_game
    ${SNESRECOMP_RUNNER_LIBRARIES})
target_compile_definitions(xmen_headless PRIVATE
    SNES_GAME_VERSION="dev"
    SNES_GAME_TITLE="X-Men: Mutant Apocalypse")
snesrecomp_target_mmx_config(xmen_headless)
snesrecomp_target_sdl(xmen_headless)

add_executable(xmen_desktop
    ${SNESRECOMP_RUNNER_SOURCES}
    ${SNESRECOMP_ROOT}/runner/src/desktop/host_main.c
    ${SNESRECOMP_ROOT}/runner/src/desktop/host_clock.c
    ${SNESRECOMP_ROOT}/runner/src/desktop/host_relaunch.c
    ${XMEN_RUNNER_DIR}/wotg_game.c
    ${XMEN_RUNNER_DIR}/host_contract.c
    ${XMEN_RUNNER_DIR}/opengl_stub.c
    ${XMEN_RUNNER_DIR}/desktop_main.c)
target_include_directories(xmen_desktop PRIVATE
    ${SNESRECOMP_RUNNER_INCLUDE_DIRS}
    ${SNESRECOMP_ROOT}/runner/src/desktop
    ${XMEN_RUNNER_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/config")
target_link_libraries(xmen_desktop PRIVATE
    snesrecomp_game
    ${SNESRECOMP_RUNNER_LIBRARIES})
target_compile_definitions(xmen_desktop PRIVATE
    SNES_GAME_VERSION="dev"
    SNES_GAME_TITLE="X-Men: Mutant Apocalypse")
snesrecomp_target_mmx_config(xmen_desktop)
snesrecomp_target_post_mortem(xmen_desktop)
snesrecomp_target_sdl(xmen_desktop)
