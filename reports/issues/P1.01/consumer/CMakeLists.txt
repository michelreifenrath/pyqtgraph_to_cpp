cmake_minimum_required(VERSION 3.26)

project(pyqtgraph_cpp_p1_01_consumer LANGUAGES CXX)

find_package(pyqtgraph-cpp CONFIG REQUIRED)

add_executable(p1_01_package_consumer main.cpp)
target_link_libraries(p1_01_package_consumer PRIVATE pyqtgraph_cpp::pyqtgraph_cpp)
target_compile_features(p1_01_package_consumer PRIVATE cxx_std_20)

include(CTest)
add_test(NAME P1.01.package-consumer COMMAND p1_01_package_consumer)
set_tests_properties(P1.01.package-consumer PROPERTIES LABELS P1.01)
