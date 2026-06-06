cmake_minimum_required(VERSION 3.26)

project(pyqtgraph_cpp_p10_04_consumer LANGUAGES CXX)

find_package(pyqtgraph-cpp CONFIG REQUIRED)

if(NOT TARGET pyqtgraph_cpp::pyqtgraph_cpp)
    message(FATAL_ERROR "Expected imported target pyqtgraph_cpp::pyqtgraph_cpp")
endif()

add_executable(p10_04_package_consumer main.cpp)
target_link_libraries(p10_04_package_consumer PRIVATE pyqtgraph_cpp::pyqtgraph_cpp)
target_compile_features(p10_04_package_consumer PRIVATE cxx_std_20)

include(CTest)
add_test(NAME P10.04.package-consumer COMMAND p10_04_package_consumer)
set_tests_properties(P10.04.package-consumer PROPERTIES
    LABELS P10.04
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
