include_guard(GLOBAL)

function(cppqtgraph_create_warnings_target target_name)
    add_library(${target_name} INTERFACE)
    target_compile_options(${target_name} INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<CXX_COMPILER_ID:MSVC>:/permissive->
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall>
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wextra>
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wpedantic>
    )
endfunction()
