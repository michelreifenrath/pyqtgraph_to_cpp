include_guard(GLOBAL)

function(cppqtgraph_configure_options)
    option(CPPQTGRAPH_REQUIRE_QT "Require Qt 6 Core and Test for the default PGBOOT-001 test baseline" ON)
    option(CPPQTGRAPH_REQUIRE_OPENCV "Require OpenCV 4.x for the default CppQtGraph baseline" ON)
    set(CPPQTGRAPH_QT_MAJOR_VERSION "6" CACHE STRING "Qt major version used by CppQtGraph")
    mark_as_advanced(CPPQTGRAPH_QT_MAJOR_VERSION)

    if(NOT CPPQTGRAPH_QT_MAJOR_VERSION STREQUAL "6")
        message(FATAL_ERROR "CppQtGraph currently supports Qt 6 for PGBOOT-001. Set -DCPPQTGRAPH_QT_MAJOR_VERSION=6 or omit the variable.")
    endif()

    find_package(Qt6 6 COMPONENTS Core Gui Widgets Test QUIET)
    if(Qt6_FOUND AND TARGET Qt6::Core AND TARGET Qt6::Gui AND TARGET Qt6::Widgets AND TARGET Qt6::Test)
        set(_cppqtgraph_has_qt TRUE)
    else()
        set(_cppqtgraph_has_qt FALSE)
    endif()

    if(CPPQTGRAPH_REQUIRE_QT AND NOT _cppqtgraph_has_qt)
        message(FATAL_ERROR "Qt 6 Core and Test are required for the default PGBOOT-001 test baseline. Install Qt 6 with Core and Test components, or configure with -DCPPQTGRAPH_REQUIRE_QT=OFF only if intentionally skipping Qt-dependent test targets.")
    endif()

    find_package(OpenCV 4 QUIET)
    if(OpenCV_FOUND)
        set(_cppqtgraph_has_opencv TRUE)
    else()
        set(_cppqtgraph_has_opencv FALSE)
    endif()

    if(CPPQTGRAPH_REQUIRE_OPENCV AND NOT _cppqtgraph_has_opencv)
        message(FATAL_ERROR "OpenCV 4.x is part of the default CppQtGraph PGBOOT-001 baseline. Install OpenCV 4, or configure with -DCPPQTGRAPH_REQUIRE_OPENCV=OFF for environments intentionally validating only the skeleton.")
    endif()

    set(CPPQTGRAPH_HAS_QT "${_cppqtgraph_has_qt}" CACHE INTERNAL "Qt 6 Core/Test discovery result" FORCE)
    set(CPPQTGRAPH_HAS_OPENCV "${_cppqtgraph_has_opencv}" CACHE INTERNAL "OpenCV 4 discovery result" FORCE)

    message(STATUS "CppQtGraph Qt 6 Core/Test available: ${CPPQTGRAPH_HAS_QT}")
    message(STATUS "CppQtGraph OpenCV 4 available: ${CPPQTGRAPH_HAS_OPENCV}")
endfunction()
