# CompilerWarnings.cmake — CERT/MISRA-lite flags for gb_core and dependents.

function(gb_target_warnings tgt)
    if(MSVC)
        target_compile_options(${tgt} PRIVATE /W4)
    else()
        target_compile_options(${tgt} PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wformat=2
            -Wundef -Wpointer-arith -Wcast-qual -Wwrite-strings
            -fstack-protector-strong
        )
        if(GB_WERROR)
            target_compile_options(${tgt} PRIVATE -Werror)
        endif()
        target_compile_definitions(${tgt} PRIVATE $<$<CONFIG:Release>:_FORTIFY_SOURCE=2>)
    endif()
endfunction()
