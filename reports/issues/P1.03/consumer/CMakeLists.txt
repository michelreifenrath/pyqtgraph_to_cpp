cmake_minimum_required(VERSION 3.26)

project(pyqtgraph_cpp_p1_03_consumer LANGUAGES CXX)

find_package(pyqtgraph-cpp CONFIG REQUIRED)

if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "pyqtgraph-cpp package config did not replay its OpenCV dependency")
endif()
if(NOT OpenCV_INCLUDE_DIRS)
    message(FATAL_ERROR "pyqtgraph-cpp package config did not populate OpenCV_INCLUDE_DIRS")
endif()
if(NOT OpenCV_LIBS)
    message(FATAL_ERROR "pyqtgraph-cpp package config did not populate OpenCV_LIBS")
endif()

add_executable(p1_03_package_consumer main.cpp)
target_include_directories(p1_03_package_consumer PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(p1_03_package_consumer PRIVATE pyqtgraph_cpp::pyqtgraph_cpp ${OpenCV_LIBS})
target_compile_features(p1_03_package_consumer PRIVATE cxx_std_20)

include(CTest)
add_test(NAME P1.03.package-consumer COMMAND p1_03_package_consumer)
set_tests_properties(P1.03.package-consumer PROPERTIES LABELS P1.03)
