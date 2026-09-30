# FindSDL2.cmake — pkg-config first, then CMake config package.

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PC_SDL2 QUIET sdl2)
endif()

find_path(SDL2_INCLUDE_DIR SDL.h
    HINTS ${PC_SDL2_INCLUDEDIR} ${PC_SDL2_INCLUDE_DIRS}
    PATH_SUFFIXES SDL2
)
find_library(SDL2_LIBRARY
    NAMES SDL2
    HINTS ${PC_SDL2_LIBDIR} ${PC_SDL2_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2 DEFAULT_MSG SDL2_LIBRARY SDL2_INCLUDE_DIR)

if(SDL2_FOUND AND NOT TARGET SDL2::SDL2)
    add_library(SDL2::SDL2 UNKNOWN IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        IMPORTED_LOCATION "${SDL2_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
    )
endif()
