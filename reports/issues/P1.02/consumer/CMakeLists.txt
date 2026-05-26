cmake_minimum_required(VERSION 3.26)

project(pyqtgraph_cpp_p1_02_consumer LANGUAGES CXX)

find_package(pyqtgraph-cpp 0.1.0 CONFIG REQUIRED)

if(NOT TARGET pyqtgraph_cpp::pyqtgraph_cpp)
    message(FATAL_ERROR "Expected imported target pyqtgraph_cpp::pyqtgraph_cpp")
endif()

add_executable(p1_02_package_consumer main.cpp)
target_link_libraries(p1_02_package_consumer PRIVATE pyqtgraph_cpp::pyqtgraph_cpp)
target_compile_features(p1_02_package_consumer PRIVATE cxx_std_20)

include(CTest)
add_test(NAME P1.02.package-consumer COMMAND p1_02_package_consumer)
set_tests_properties(P1.02.package-consumer PROPERTIES LABELS P1.02)
