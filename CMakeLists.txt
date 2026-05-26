cmake_minimum_required(VERSION 3.26)

project(pyqtgraph_cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
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
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(pyqtgraph_cpp
    PUBLIC
        pyqtgraph_cpp_project_options
)
target_compile_options(pyqtgraph_cpp
    PRIVATE
        $<TARGET_PROPERTY:pyqtgraph_cpp_project_warnings,INTERFACE_COMPILE_OPTIONS>
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
            src/pyqtgraph/graphicsItems/PlotItem/PlotItem.cpp
            src/pyqtgraph/widgets/PlotWidget.cpp
            src/pyqtgraph/graphicsItems/PlotCurveItem.cpp
    )
    set_target_properties(pyqtgraph_cpp PROPERTIES AUTOMOC ON)
    target_link_libraries(pyqtgraph_cpp
        PUBLIC
            Qt6::Widgets
    )
endif()

set(_pyqtgraph_cpp_public_qt_components)
if(_pyqtgraph_cpp_has_qtcore)
    list(APPEND _pyqtgraph_cpp_public_qt_components Core)
endif()
if(_pyqtgraph_cpp_has_vector OR _pyqtgraph_cpp_has_mkcolor)
    list(APPEND _pyqtgraph_cpp_public_qt_components Gui)
endif()
if(_pyqtgraph_cpp_has_graphicsitem)
    list(APPEND _pyqtgraph_cpp_public_qt_components Widgets)
endif()
list(REMOVE_DUPLICATES _pyqtgraph_cpp_public_qt_components)
set(PYQTGRAPH_CPP_PACKAGE_QT_COMPONENTS "${_pyqtgraph_cpp_public_qt_components}")
set(PYQTGRAPH_CPP_PACKAGE_REQUIRE_OPENCV "${PYQTGRAPH_CPP_HAS_OPENCV}")

set(_pyqtgraph_cpp_package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/pyqtgraph-cpp")
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/pyqtgraph-cppConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/pyqtgraph-cppConfig.cmake"
    INSTALL_DESTINATION "${_pyqtgraph_cpp_package_dir}"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/pyqtgraph-cppConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

install(TARGETS
    pyqtgraph_cpp
    pyqtgraph_cpp_project_options
    EXPORT pyqtgraph-cppTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.hpp"
)
install(EXPORT pyqtgraph-cppTargets
    FILE pyqtgraph-cppTargets.cmake
    NAMESPACE pyqtgraph_cpp::
    DESTINATION "${_pyqtgraph_cpp_package_dir}"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/pyqtgraph-cppConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/pyqtgraph-cppConfigVersion.cmake"
    DESTINATION "${_pyqtgraph_cpp_package_dir}"
)

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    add_subdirectory(tests)

    add_test(
        NAME P1.06.visual.smoke
        COMMAND "${Python3_EXECUTABLE}" -m pytest -q
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/visual/test_P1_06_visual_smoke.py"
    )
    set_tests_properties(
        P1.06.visual.smoke
        PROPERTIES
            LABELS "visual;P1.06"
            ENVIRONMENT "PG_VISUAL_ARTIFACT_DIR=${CMAKE_CURRENT_BINARY_DIR}/reports/visual/P1.06"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_executable(pyqtgraph_cpp_oracle_P0_06 tests/oracle/P0_06_oracle_comparison.cpp)
    target_link_libraries(pyqtgraph_cpp_oracle_P0_06
        PRIVATE
            pyqtgraph_cpp_project_options
            pyqtgraph_cpp_project_warnings
    )
    target_compile_definitions(pyqtgraph_cpp_oracle_P0_06
        PRIVATE
            PYQTGRAPH_CPP_P0_06_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/oracle/fixtures/P0_06/probe_contract.json"
    )
    pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_oracle_P0_06)
    add_test(NAME pyqtgraph_cpp.oracle.P0_06 COMMAND pyqtgraph_cpp_oracle_P0_06)
    set_tests_properties(pyqtgraph_cpp.oracle.P0_06 PROPERTIES LABELS P0.06)
    add_test(NAME pyqtgraph_cpp.oracle.P0_06.mismatch COMMAND pyqtgraph_cpp_oracle_P0_06 --mismatch)
    set_tests_properties(pyqtgraph_cpp.oracle.P0_06.mismatch PROPERTIES LABELS P0.06 WILL_FAIL TRUE)

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

        add_executable(pyqtgraph_cpp_graphicsitems_viewbox_range tests/graphicsItems/test_ViewBox_range.cpp)
        target_link_libraries(pyqtgraph_cpp_graphicsitems_viewbox_range
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_graphicsitems_viewbox_range)
        add_test(NAME pyqtgraph_cpp.graphicsItems.ViewBox.range COMMAND pyqtgraph_cpp_graphicsitems_viewbox_range)

        add_executable(pyqtgraph_cpp_graphicsitems_viewbox_pan_zoom tests/graphicsItems/test_ViewBox_pan_zoom.cpp)
        target_link_libraries(pyqtgraph_cpp_graphicsitems_viewbox_pan_zoom
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        target_compile_definitions(pyqtgraph_cpp_graphicsitems_viewbox_pan_zoom
            PRIVATE
                PYQTGRAPH_CPP_INTERACTION_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/oracle/fixtures/interactions"
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_graphicsitems_viewbox_pan_zoom)
        add_test(NAME pyqtgraph_cpp.graphicsItems.ViewBox.pan_zoom COMMAND pyqtgraph_cpp_graphicsitems_viewbox_pan_zoom)

        add_executable(pyqtgraph_cpp_graphicsitems_plotitem tests/graphicsItems/test_PlotItem.cpp)
        target_link_libraries(pyqtgraph_cpp_graphicsitems_plotitem
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_graphicsitems_plotitem)
        add_test(NAME pyqtgraph_cpp.graphicsItems.PlotItem COMMAND pyqtgraph_cpp_graphicsitems_plotitem)

        add_executable(pyqtgraph_cpp_widgets_plotwidget tests/widgets/test_PlotWidget.cpp)
        target_link_libraries(pyqtgraph_cpp_widgets_plotwidget
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_widgets_plotwidget)
        add_test(NAME pyqtgraph_cpp.widgets.PlotWidget COMMAND pyqtgraph_cpp_widgets_plotwidget)

        add_executable(pyqtgraph_cpp_graphicsitems_plotcurveitem tests/graphicsItems/test_PlotCurveItem.cpp)
        target_link_libraries(pyqtgraph_cpp_graphicsitems_plotcurveitem
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_graphicsitems_plotcurveitem)
        add_test(NAME pyqtgraph_cpp.graphicsItems.PlotCurveItem COMMAND pyqtgraph_cpp_graphicsitems_plotcurveitem)

        add_executable(pyqtgraph_cpp_graphicsitems_plotcurveitem_setdata tests/graphicsItems/test_PlotCurveItem_setData.cpp)
        target_link_libraries(pyqtgraph_cpp_graphicsitems_plotcurveitem_setdata
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_graphicsitems_plotcurveitem_setdata)
        add_test(NAME pyqtgraph_cpp.graphicsItems.PlotCurveItem.setData COMMAND pyqtgraph_cpp_graphicsitems_plotcurveitem_setdata)

        add_executable(pyqtgraph_cpp_examples_simpleplot examples/SimplePlot.cpp)
        target_link_libraries(pyqtgraph_cpp_examples_simpleplot
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_examples_simpleplot)

        add_executable(pyqtgraph_cpp_visual_render_example tests/visual/P1_08_render_cpp_example.cpp)
        target_link_libraries(pyqtgraph_cpp_visual_render_example
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_visual_render_example)
        add_test(
            NAME P1.08.visual.SimplePlot
            COMMAND "${Python3_EXECUTABLE}" -m pytest -q
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/visual/test_P1_08_cpp_visual_renderer.py"
        )
        set_tests_properties(
            P1.08.visual.SimplePlot
            PROPERTIES
                LABELS "visual;P1.08"
                ENVIRONMENT "QT_QPA_PLATFORM=offscreen;PG_CPP_VISUAL_RENDERER=$<TARGET_FILE:pyqtgraph_cpp_visual_render_example>;PG_VISUAL_REPORTS_ROOT=${CMAKE_CURRENT_BINARY_DIR}/reports/visual/P1.08;PG_VISUAL_REVIEW_REPORT=${CMAKE_CURRENT_SOURCE_DIR}/reports/visual/P1.08/SimplePlot/gpt5_vision_review.md"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )

        add_executable(pyqtgraph_cpp_examples_simpleplot_test tests/examples/test_SimplePlot.cpp)
        target_link_libraries(pyqtgraph_cpp_examples_simpleplot_test
            PRIVATE
                pyqtgraph_cpp
                pyqtgraph_cpp_project_options
                pyqtgraph_cpp_project_warnings
        )
        pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_examples_simpleplot_test)
        add_test(NAME pyqtgraph_cpp.examples.SimplePlot COMMAND pyqtgraph_cpp_examples_simpleplot_test)

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

    add_test(
        NAME P0.08.performance.schema.create_baseline
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_performance"
            --mode create-baseline
            --baseline "${CMAKE_CURRENT_BINARY_DIR}/reports/performance/P0.08/create-baseline.json"
            --output "${CMAKE_CURRENT_BINARY_DIR}/reports/performance/P0.08/create-metrics.json"
    )
    add_test(
        NAME P0.08.performance.schema.update_baseline
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_performance"
            --mode update-baseline
            --baseline "${CMAKE_CURRENT_BINARY_DIR}/reports/performance/P0.08/update-baseline.json"
            --output "${CMAKE_CURRENT_BINARY_DIR}/reports/performance/P0.08/update-metrics.json"
            --rationale "P0.08 focused proof exercises explicit baseline update mode."
    )
    add_test(
        NAME P0.08.performance.schema
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_performance"
            --mode compare
            --baseline "${CMAKE_CURRENT_SOURCE_DIR}/reports/performance/P0.08/baseline.json"
            --output "${CMAKE_CURRENT_BINARY_DIR}/reports/performance/P0.08/metrics.json"
            --synthetic-regression-check
    )
    set_tests_properties(
        P0.08.performance.schema.create_baseline
        P0.08.performance.schema.update_baseline
        P0.08.performance.schema
        PROPERTIES LABELS "performance;P0.08"
    )
endif()
