cmake_minimum_required(VERSION 3.26)

project(pyqtgraph_cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(PyQtGraphCppOptions)
include(PyQtGraphCppWarnings)
include(PyQtGraphCppSanitizers)

pyqtgraph_cpp_configure_options()

add_library(pyqtgraph_cpp_project_options INTERFACE)
target_compile_features(pyqtgraph_cpp_project_options INTERFACE cxx_std_20)

pyqtgraph_cpp_create_warnings_target(pyqtgraph_cpp_project_warnings)

add_library(pyqtgraph_cpp
    src/pyqtgraph/core/ArrayView.cpp
    src/pyqtgraph/functions.cpp
)
target_include_directories(pyqtgraph_cpp
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(pyqtgraph_cpp
    PUBLIC
        pyqtgraph_cpp_project_options
        Qt6::Core
        Qt6::Gui
    PRIVATE
        pyqtgraph_cpp_project_warnings
)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)

    add_executable(pyqtgraph_cpp_core_arrayview tests/core/test_ArrayView.cpp)
    target_link_libraries(pyqtgraph_cpp_core_arrayview
        PRIVATE
            pyqtgraph_cpp
            pyqtgraph_cpp_project_options
            pyqtgraph_cpp_project_warnings
    )
    pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_core_arrayview)
    add_test(NAME pyqtgraph_cpp.core.ArrayView COMMAND pyqtgraph_cpp_core_arrayview)

    add_executable(pyqtgraph_cpp_core_mkcolor tests/core/test_mkColor.cpp)
    target_link_libraries(pyqtgraph_cpp_core_mkcolor
        PRIVATE
            pyqtgraph_cpp
            pyqtgraph_cpp_project_options
            pyqtgraph_cpp_project_warnings
            Qt6::Gui
    )
    pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_core_mkcolor)
    add_test(NAME pyqtgraph_cpp.core.mkColor COMMAND pyqtgraph_cpp_core_mkcolor)
endif()
