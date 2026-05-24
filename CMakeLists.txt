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

find_package(Qt6 6 COMPONENTS Core Gui Widgets QUIET)
set(_pyqtgraph_cpp_has_qtcore FALSE)
if(TARGET Qt6::Core)
    set(_pyqtgraph_cpp_has_qtcore TRUE)
elseif(PYQTGRAPH_CPP_REQUIRE_QT)
    message(FATAL_ERROR "Qt 6 Core is required for PGCORE-003 Point. Install Qt 6 Core, or configure with -DPYQTGRAPH_CPP_REQUIRE_QT=OFF only if intentionally skipping Qt-dependent targets.")
else()
    message(STATUS "Qt6 Core was not found; skipping the Point library source and test target")
endif()

set(_pyqtgraph_cpp_has_vector FALSE)
if(TARGET Qt6::Core AND TARGET Qt6::Gui)
    set(_pyqtgraph_cpp_has_vector TRUE)
elseif(PYQTGRAPH_CPP_REQUIRE_QT)
    message(FATAL_ERROR "Qt 6 Gui is required for PGCORE-004 Vector. Install Qt 6 Gui, or configure with -DPYQTGRAPH_CPP_REQUIRE_QT=OFF only if intentionally skipping Qt-dependent targets.")
else()
    message(STATUS "Qt6 Core/Gui was not found; skipping the Vector library source and test target")
endif()

set(_pyqtgraph_cpp_has_mkcolor FALSE)
if(TARGET Qt6::Core AND TARGET Qt6::Gui)
    set(_pyqtgraph_cpp_has_mkcolor TRUE)
elseif(PYQTGRAPH_CPP_REQUIRE_QT)
    message(FATAL_ERROR "Qt 6 Gui is required for PGCORE-005 mkColor. Install Qt 6 Gui, or configure with -DPYQTGRAPH_CPP_REQUIRE_QT=OFF only if intentionally skipping Qt-dependent targets.")
else()
    message(STATUS "Qt6 Core/Gui was not found; skipping the mkColor library source and test target")
endif()

set(_pyqtgraph_cpp_has_graphicsitem FALSE)
if(TARGET Qt6::Core AND TARGET Qt6::Gui AND TARGET Qt6::Widgets)
    set(_pyqtgraph_cpp_has_graphicsitem TRUE)
elseif(PYQTGRAPH_CPP_REQUIRE_QT)
    message(FATAL_ERROR "Qt 6 Widgets is required for PGGI-001 GraphicsItem and PGGI-002 GraphicsObject. Install Qt 6 Widgets, or configure with -DPYQTGRAPH_CPP_REQUIRE_QT=OFF only if intentionally skipping Qt Widgets-dependent targets.")
else()
    message(STATUS "Qt6 Widgets was not found; skipping the GraphicsItem/GraphicsObject library sources and test targets")
endif()

add_library(pyqtgraph_cpp
    src/pyqtgraph/core/ArrayView.cpp
    src/pyqtgraph/functions.cpp
    src/pyqtgraph/PlotData.cpp
)
target_include_directories(pyqtgraph_cpp
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(pyqtgraph_cpp
    PUBLIC
        pyqtgraph_cpp_project_options
    PRIVATE
        pyqtgraph_cpp_project_warnings
)
if(_pyqtgraph_cpp_has_qtcore)
    target_sources(pyqtgraph_cpp
        PRIVATE
            src/pyqtgraph/Point.cpp
    )
    target_link_libraries(pyqtgraph_cpp
        PUBLIC
            Qt6::Core
    )
endif()

if(_pyqtgraph_cpp_has_vector)
    target_sources(pyqtgraph_cpp
        PRIVATE
            src/pyqtgraph/Vector.cpp
    )
    target_link_libraries(pyqtgraph_cpp
        PUBLIC
            Qt6::Gui
    )
endif()

if(_pyqtgraph_cpp_has_mkcolor)
    target_sources(pyqtgraph_cpp
        PRIVATE
            src/pyqtgraph/colormap.cpp
            src/pyqtgraph/functions_qimage.cpp
    )
    target_compile_definitions(pyqtgraph_cpp
        PUBLIC
            PYQTGRAPH_CPP_ENABLE_QT_COLOR=1
    )
    target_link_libraries(pyqtgraph_cpp
        PUBLIC
            Qt6::Gui
    )
endif()

if(_pyqtgraph_cpp_has_graphicsitem)
    target_sources(pyqtgraph_cpp
        PRIVATE
            include/pyqtgraph/GraphicsScene/GraphicsScene.hpp
            include/pyqtgraph/GraphicsScene/mouseEvents.hpp
            src/pyqtgraph/GraphicsScene/GraphicsScene.cpp
            src/pyqtgraph/GraphicsScene/mouseEvents.cpp
            src/pyqtgraph/graphicsItems/GraphicsItem.cpp
            src/pyqtgraph/graphicsItems/GraphicsObject.cpp
            src/pyqtgraph/graphicsItems/GraphicsWidget.cpp
            src/pyqtgraph/graphicsItems/AxisItem.cpp
            src/pyqtgraph/graphicsItems/ViewBox/ViewBox.cpp
    )
    set_target_properties(pyqtgraph_cpp PROPERTIES AUTOMOC ON)
    target_link_libraries(pyqtgraph_cpp
        PUBLIC
            Qt6::Widgets
    )
endif()

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

    add_executable(pyqtgraph_cpp_core_nan_minmax tests/core/test_nan_minmax.cpp)
    target_link_libraries(pyqtgraph_cpp_core_nan_minmax
        PRIVATE
            pyqtgraph_cpp
            pyqtgraph_cpp_project_options
            pyqtgraph_cpp_project_warnings
    )
    pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_core_nan_minmax)
    add_test(NAME pyqtgraph_cpp.core.nan_minmax COMMAND pyqtgraph_cpp_core_nan_minmax)

    add_executable(pyqtgraph_cpp_core_plotdata tests/core/test_PlotData.cpp)
    target_link_libraries(pyqtgraph_cpp_core_plotdata
        PRIVATE
            pyqtgraph_cpp
            pyqtgraph_cpp_project_options
            pyqtgraph_cpp_project_warnings
    )
    pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_core_plotdata)
    add_test(NAME pyqtgraph_cpp.core.PlotData COMMAND pyqtgraph_cpp_core_plotdata)

    if(_pyqtgraph_cpp_has_qtcore)
        add_executable(pyqtgraph_cpp_core_point tests/core/test_Point.cpp)
        target_link_libraries(pyqtgraph_cpp_core_point
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_core_point)
        add_test(NAME pyqtgraph_cpp.core.Point COMMAND pyqtgraph_cpp_core_point)
    endif()

    if(_pyqtgraph_cpp_has_vector)
        add_executable(pyqtgraph_cpp_core_vector tests/core/test_Vector.cpp)
        target_link_libraries(pyqtgraph_cpp_core_vector
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_core_vector)
        add_test(NAME pyqtgraph_cpp.core.Vector COMMAND pyqtgraph_cpp_core_vector)
    endif()

    if(_pyqtgraph_cpp_has_mkcolor)
        add_executable(pyqtgraph_cpp_core_mkcolor tests/core/test_mkColor.cpp)
        target_link_libraries(pyqtgraph_cpp_core_mkcolor
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_core_mkcolor)
        add_test(NAME pyqtgraph_cpp.core.mkColor COMMAND pyqtgraph_cpp_core_mkcolor)

        add_executable(pyqtgraph_cpp_core_makeqimage tests/core/test_makeQImage.cpp)
        target_link_libraries(pyqtgraph_cpp_core_makeqimage
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_core_makeqimage)
        add_test(NAME pyqtgraph_cpp.core.makeQImage COMMAND pyqtgraph_cpp_core_makeqimage)

        add_executable(pyqtgraph_cpp_core_colormap tests/core/test_ColorMap.cpp)
        target_link_libraries(pyqtgraph_cpp_core_colormap
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_core_colormap)
        add_test(NAME pyqtgraph_cpp.core.ColorMap COMMAND pyqtgraph_cpp_core_colormap)

        add_executable(pyqtgraph_cpp_core_colormap_lut tests/core/test_ColorMap_LUT.cpp)
        target_link_libraries(pyqtgraph_cpp_core_colormap_lut
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_core_colormap_lut)
        add_test(NAME pyqtgraph_cpp.core.ColorMap.LUT COMMAND pyqtgraph_cpp_core_colormap_lut)

        add_executable(pyqtgraph_cpp_core_mkpen_mkbrush tests/core/test_mkPen_mkBrush.cpp)
        target_link_libraries(pyqtgraph_cpp_core_mkpen_mkbrush
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_core_mkpen_mkbrush)
        add_test(NAME pyqtgraph_cpp.core.mkPen_mkBrush COMMAND pyqtgraph_cpp_core_mkpen_mkbrush)
    endif()

    if(_pyqtgraph_cpp_has_graphicsitem)
        add_executable(pyqtgraph_cpp_graphicsitems_graphicsitem tests/graphicsItems/test_GraphicsItem.cpp)
        target_link_libraries(pyqtgraph_cpp_graphicsitems_graphicsitem
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_graphicsitems_graphicsitem)
        add_test(NAME pyqtgraph_cpp.graphicsItems.GraphicsItem COMMAND pyqtgraph_cpp_graphicsitems_graphicsitem)

        add_executable(pyqtgraph_cpp_graphicsitems_graphicsobject tests/graphicsItems/test_GraphicsObject.cpp)
        target_link_libraries(pyqtgraph_cpp_graphicsitems_graphicsobject
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_graphicsitems_graphicsobject)
        add_test(NAME pyqtgraph_cpp.graphicsItems.GraphicsObject COMMAND pyqtgraph_cpp_graphicsitems_graphicsobject)

        add_executable(pyqtgraph_cpp_graphicsitems_graphicswidget tests/graphicsItems/test_GraphicsWidget.cpp)
        target_link_libraries(pyqtgraph_cpp_graphicsitems_graphicswidget
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_graphicsitems_graphicswidget)
        add_test(NAME pyqtgraph_cpp.graphicsItems.GraphicsWidget COMMAND pyqtgraph_cpp_graphicsitems_graphicswidget)

        add_executable(pyqtgraph_cpp_graphicsscene_graphicsscene tests/graphicsItems/test_GraphicsScene.cpp)
        set_target_properties(pyqtgraph_cpp_graphicsscene_graphicsscene PROPERTIES AUTOMOC ON)
        target_link_libraries(pyqtgraph_cpp_graphicsscene_graphicsscene
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_graphicsscene_graphicsscene)
        add_test(NAME pyqtgraph_cpp.GraphicsScene.GraphicsScene COMMAND pyqtgraph_cpp_graphicsscene_graphicsscene)

        add_executable(pyqtgraph_cpp_graphicsscene_mouseevents tests/graphicsItems/test_mouseEvents.cpp)
        target_link_libraries(pyqtgraph_cpp_graphicsscene_mouseevents
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_graphicsscene_mouseevents)
        add_test(NAME pyqtgraph_cpp.GraphicsScene.mouseEvents COMMAND pyqtgraph_cpp_graphicsscene_mouseevents)

        add_executable(pyqtgraph_cpp_graphicsitems_axisitem tests/graphicsItems/test_AxisItem.cpp)
        target_link_libraries(pyqtgraph_cpp_graphicsitems_axisitem
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_graphicsitems_axisitem)
        add_test(NAME pyqtgraph_cpp.graphicsItems.AxisItem COMMAND pyqtgraph_cpp_graphicsitems_axisitem)

        add_executable(pyqtgraph_cpp_graphicsitems_viewbox tests/graphicsItems/test_ViewBox.cpp)
        target_link_libraries(pyqtgraph_cpp_graphicsitems_viewbox
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_graphicsitems_viewbox)
        add_test(NAME pyqtgraph_cpp.graphicsItems.ViewBox COMMAND pyqtgraph_cpp_graphicsitems_viewbox)

        add_executable(pyqtgraph_cpp_hierarchy_cpp tests/hierarchy/test_cpp_hierarchy.cpp)
        target_link_libraries(pyqtgraph_cpp_hierarchy_cpp
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_hierarchy_cpp)
        add_test(NAME pyqtgraph_cpp.hierarchy.cpp COMMAND pyqtgraph_cpp_hierarchy_cpp)
    endif()
endif()
