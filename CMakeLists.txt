cmake_minimum_required(VERSION 3.16)
project(gameboy-emulator C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GB_BUILD_TESTS "Build unit/integration tests" ON)
option(GB_BUILD_DEBUGGER "Include debugger in the core (always on for desktop)" ON)
option(GB_ENABLE_SANITIZERS "Link ASan/UBSan (see GB_SANITIZER)" OFF)
set(GB_SANITIZER "address,undefined" CACHE STRING "Sanitizer set")
option(GB_BUILD_ANDROID "Configure Android NDK stub only" OFF)
option(GB_BUILD_IOS "Configure iOS stub only" OFF)
option(GB_WERROR "Treat warnings as errors" ON)
option(GB_BUILD_DESKTOP "SDL2 frontend" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(Sanitizers)

set(GB_CORE_SOURCES
    src/cpu/cpu.c
    src/cpu/cpu_decode.c
    src/cpu/cpu_execute.c
    src/cpu/cpu_cb.c
    src/cpu/cpu_flags.c
    src/mmu/mmu.c
    src/mmu/mmu_io.c
    src/mmu/dma.c
    src/ppu/ppu.c
    src/ppu/ppu_bg.c
    src/ppu/ppu_window.c
    src/ppu/ppu_sprite.c
    src/apu/apu.c
    src/timer/timer.c
    src/joypad/joypad.c
    src/cartridge/cartridge.c
    src/cartridge/mbc.c
    src/cartridge/mbc_none.c
    src/cartridge/mbc1.c
    src/cartridge/mbc2.c
    src/cartridge/mbc3.c
    src/cartridge/mbc5.c
    src/interrupts/interrupts.c
    src/debugger/debugger.c
    src/debugger/disassembler.c
)

add_library(gb_core STATIC ${GB_CORE_SOURCES})
target_include_directories(gb_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
gb_target_warnings(gb_core)
gb_enable_sanitizers(gb_core)
if(UNIX AND NOT APPLE)
    target_link_libraries(gb_core PUBLIC m)
endif()

if(GB_BUILD_DESKTOP AND NOT GB_BUILD_ANDROID AND NOT GB_BUILD_IOS)
    find_package(SDL2 REQUIRED)
    add_executable(gb_desktop src/main.c)
    target_link_libraries(gb_desktop PRIVATE gb_core SDL2::SDL2)
    gb_target_warnings(gb_desktop)
    gb_enable_sanitizers(gb_desktop)
    if(UNIX AND NOT APPLE)
        target_link_options(gb_desktop PRIVATE -pie)
        target_compile_options(gb_desktop PRIVATE -fPIE)
    endif()
endif()

if(GB_BUILD_TESTS)
    add_executable(gb_tests
        tests/unit/test_cpu.c
        tests/unit/test_mmu.c
        tests/unit/test_ppu.c
        tests/unit/test_mbc.c
    )
    target_include_directories(gb_tests PRIVATE tests/framework include)
    target_link_libraries(gb_tests PRIVATE gb_core)
    gb_target_warnings(gb_tests)
    gb_enable_sanitizers(gb_tests)
    enable_testing()
    add_test(NAME gb_unit COMMAND gb_tests)
endif()

if(GB_BUILD_ANDROID)
    add_subdirectory(android)
endif()
