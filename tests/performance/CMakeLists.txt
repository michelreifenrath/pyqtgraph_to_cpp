add_executable(test_P1_07_performance_preset test_P1_07_performance_preset.cpp)
target_link_libraries(test_P1_07_performance_preset
    PRIVATE
        pyqtgraph_cpp_project_options
        pyqtgraph_cpp_project_warnings
)
pyqtgraph_cpp_enable_sanitizers(test_P1_07_performance_preset)

add_test(NAME P1.07.performance.smoke COMMAND test_P1_07_performance_preset)
set_tests_properties(P1.07.performance.smoke PROPERTIES LABELS "performance;P1.07")
