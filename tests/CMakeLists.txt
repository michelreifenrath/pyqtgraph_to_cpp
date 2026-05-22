add_executable(pyqtgraph_cpp_smoke_empty smoke/test_empty.cpp)
target_link_libraries(pyqtgraph_cpp_smoke_empty
    PRIVATE
        pyqtgraph_cpp_project_options
        pyqtgraph_cpp_project_warnings
)
pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_smoke_empty)

add_test(NAME pyqtgraph_cpp.smoke.empty COMMAND pyqtgraph_cpp_smoke_empty)

if(NOT PYQTGRAPH_CPP_HAS_QT)
    message(STATUS "Qt6 was not found; running only the dependency-free CTest smoke target")
endif()
