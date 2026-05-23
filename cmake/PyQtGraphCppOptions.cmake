include_guard(GLOBAL)

function(pyqtgraph_cpp_configure_options)
    option(PYQTGRAPH_CPP_REQUIRE_QT "Require Qt 6 Core, Gui, and Test for the default PGBOOT-001 test baseline" ON)
    option(PYQTGRAPH_CPP_REQUIRE_OPENCV "Require OpenCV 4.x for the default pyqtgraph-cpp baseline" ON)
    set(PYQTGRAPH_CPP_QT_MAJOR_VERSION "6" CACHE STRING "Qt major version used by pyqtgraph-cpp")
    mark_as_advanced(PYQTGRAPH_CPP_QT_MAJOR_VERSION)

    if(NOT PYQTGRAPH_CPP_QT_MAJOR_VERSION STREQUAL "6")
        message(FATAL_ERROR "pyqtgraph-cpp currently supports Qt 6 for PGBOOT-001. Set -DPYQTGRAPH_CPP_QT_MAJOR_VERSION=6 or omit the variable.")
    endif()

    find_package(Qt6 6 COMPONENTS Core Gui Test QUIET)
    if(Qt6_FOUND AND TARGET Qt6::Core AND TARGET Qt6::Gui AND TARGET Qt6::Test)
        set(_pyqtgraph_cpp_has_qt TRUE)
    else()
        set(_pyqtgraph_cpp_has_qt FALSE)
    endif()

    if(PYQTGRAPH_CPP_REQUIRE_QT AND NOT _pyqtgraph_cpp_has_qt)
        message(FATAL_ERROR "Qt 6 Core, Gui, and Test are required for the default PGBOOT-001 test baseline. Install Qt 6 with Core, Gui, and Test components, or configure with -DPYQTGRAPH_CPP_REQUIRE_QT=OFF only if intentionally skipping Qt-dependent test targets.")
    endif()

    find_package(OpenCV 4 QUIET)
    if(OpenCV_FOUND)
        set(_pyqtgraph_cpp_has_opencv TRUE)
    else()
        set(_pyqtgraph_cpp_has_opencv FALSE)
    endif()

    if(PYQTGRAPH_CPP_REQUIRE_OPENCV AND NOT _pyqtgraph_cpp_has_opencv)
        message(FATAL_ERROR "OpenCV 4.x is part of the default pyqtgraph-cpp PGBOOT-001 baseline. Install OpenCV 4, or configure with -DPYQTGRAPH_CPP_REQUIRE_OPENCV=OFF for environments intentionally validating only the skeleton.")
    endif()

    set(PYQTGRAPH_CPP_HAS_QT "${_pyqtgraph_cpp_has_qt}" CACHE INTERNAL "Qt 6 Core/Gui/Test discovery result" FORCE)
    set(PYQTGRAPH_CPP_HAS_OPENCV "${_pyqtgraph_cpp_has_opencv}" CACHE INTERNAL "OpenCV 4 discovery result" FORCE)

    message(STATUS "pyqtgraph-cpp Qt 6 Core/Gui/Test available: ${PYQTGRAPH_CPP_HAS_QT}")
    message(STATUS "pyqtgraph-cpp OpenCV 4 available: ${PYQTGRAPH_CPP_HAS_OPENCV}")
endfunction()
