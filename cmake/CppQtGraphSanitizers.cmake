include_guard(GLOBAL)

option(CPPQTGRAPH_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer for supported compilers" OFF)

function(cppqtgraph_enable_sanitizers target_name)
    if(NOT CPPQTGRAPH_ENABLE_SANITIZERS)
        return()
    endif()

    if(MSVC)
        message(WARNING "CPPQTGRAPH_ENABLE_SANITIZERS is not supported for MSVC in this PGBOOT-001 baseline; continuing without sanitizer flags for ${target_name}.")
        return()
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target_name} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target_name} PRIVATE -fsanitize=address,undefined)
    else()
        message(WARNING "CPPQTGRAPH_ENABLE_SANITIZERS is not supported for compiler '${CMAKE_CXX_COMPILER_ID}'; continuing without sanitizer flags for ${target_name}.")
    endif()
endfunction()
