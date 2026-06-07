function(pyqtgraph_cpp_configure_cpp_test_target target)
    target_link_libraries(${target}
        PRIVATE
            pyqtgraph_cpp
            pyqtgraph_cpp_project_options
            pyqtgraph_cpp_project_warnings
    )
    pyqtgraph_cpp_enable_sanitizers(${target})
endfunction()

function(pyqtgraph_cpp_add_cpp_test target source test_name)
    add_executable(${target} ${source})
    pyqtgraph_cpp_configure_cpp_test_target(${target})
    add_test(NAME ${test_name} COMMAND ${target})
endfunction()

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

add_test(
    NAME P1.06.visual.smoke
    COMMAND "${Python3_EXECUTABLE}" -m pytest -q
        "${PROJECT_SOURCE_DIR}/tests/visual/test_P1_06_visual_smoke.py"
)
set_tests_properties(
    P1.06.visual.smoke
    PROPERTIES
        LABELS "visual;P1.06"
        ENVIRONMENT "PG_VISUAL_ARTIFACT_DIR=${PROJECT_BINARY_DIR}/reports/visual/P1.06"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_executable(pyqtgraph_cpp_oracle_P0_06 oracle/P0_06_oracle_comparison.cpp)
target_link_libraries(pyqtgraph_cpp_oracle_P0_06
    PRIVATE
        pyqtgraph_cpp_project_options
        pyqtgraph_cpp_project_warnings
)
target_compile_definitions(pyqtgraph_cpp_oracle_P0_06
    PRIVATE
        PYQTGRAPH_CPP_P0_06_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P0_06/probe_contract.json"
)
pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_oracle_P0_06)
add_test(NAME pyqtgraph_cpp.oracle.P0_06 COMMAND pyqtgraph_cpp_oracle_P0_06)
set_tests_properties(pyqtgraph_cpp.oracle.P0_06 PROPERTIES LABELS P0.06)
add_test(NAME pyqtgraph_cpp.oracle.P0_06.mismatch COMMAND pyqtgraph_cpp_oracle_P0_06 --mismatch)
set_tests_properties(pyqtgraph_cpp.oracle.P0_06.mismatch PROPERTIES LABELS P0.06 WILL_FAIL TRUE)

add_test(
    NAME pyqtgraph_cpp.oracle.P2_01
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/oracle/probes/P2_01_point_vector_oracle.py" --check
)
set_tests_properties(
    pyqtgraph_cpp.oracle.P2_01
    PROPERTIES
        LABELS P2.01
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_test(
    NAME pyqtgraph_cpp.oracle.P2_08
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/oracle/scripts/generate_P2_08_signal_proxy_timer_oracle.py" --check
)
set_tests_properties(
    pyqtgraph_cpp.oracle.P2_08
    PROPERTIES
        LABELS P2.08
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_test(
    NAME pyqtgraph_cpp.oracle.P2_03
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/oracle/probes/P2_03_plotdata_oracle.py" --check
)
set_tests_properties(
    pyqtgraph_cpp.oracle.P2_03
    PROPERTIES
        LABELS P2.03
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_test(
    NAME pyqtgraph_cpp.oracle.P2_04
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/oracle/probes/P2_04_functions_symbol_oracle.py" --check
)
set_tests_properties(
    pyqtgraph_cpp.oracle.P2_04
    PROPERTIES
        LABELS P2.04
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_test(
    NAME pyqtgraph_cpp.oracle.P2_06
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/oracle/probes/P2_06_colormap_oracle.py" --check
)
set_tests_properties(
    pyqtgraph_cpp.oracle.P2_06
    PROPERTIES
        LABELS P2.06
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

pyqtgraph_cpp_add_cpp_test(pyqtgraph_cpp_core_arrayview core/test_ArrayView.cpp pyqtgraph_cpp.core.ArrayView)
pyqtgraph_cpp_add_cpp_test(pyqtgraph_cpp_core_nan_minmax core/test_nan_minmax.cpp pyqtgraph_cpp.core.nan_minmax)
pyqtgraph_cpp_add_cpp_test(pyqtgraph_cpp_core_plotdata core/test_PlotData.cpp pyqtgraph_cpp.core.PlotData)
target_compile_definitions(pyqtgraph_cpp_core_plotdata
    PRIVATE
        PYQTGRAPH_CPP_P2_03_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P2_03/plotdata_oracle.json"
)
set_tests_properties(pyqtgraph_cpp.core.PlotData PROPERTIES LABELS P2.03)

if(_pyqtgraph_cpp_has_qtcore)
    pyqtgraph_cpp_add_cpp_test(pyqtgraph_cpp_core_point core/test_Point.cpp pyqtgraph_cpp.core.Point)
    target_compile_definitions(pyqtgraph_cpp_core_point
        PRIVATE
            PYQTGRAPH_CPP_P2_01_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P2_01/point_vector_oracle.json"
    )
    set_tests_properties(pyqtgraph_cpp.core.Point PROPERTIES LABELS P2.01)

    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_core_signalproxy_threadsafe_timer
        core/test_SignalProxy_ThreadsafeTimer.cpp
        pyqtgraph_cpp.core.SignalProxy_ThreadsafeTimer
    )
    target_compile_definitions(pyqtgraph_cpp_core_signalproxy_threadsafe_timer
        PRIVATE
            PYQTGRAPH_CPP_P2_08_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P2_08/signal_proxy_timer_oracle.json"
    )
    set_tests_properties(pyqtgraph_cpp.core.SignalProxy_ThreadsafeTimer PROPERTIES LABELS P2.08)
endif()

if(_pyqtgraph_cpp_has_vector)
    pyqtgraph_cpp_add_cpp_test(pyqtgraph_cpp_core_vector core/test_Vector.cpp pyqtgraph_cpp.core.Vector)
    target_compile_definitions(pyqtgraph_cpp_core_vector
        PRIVATE
            PYQTGRAPH_CPP_P2_01_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P2_01/point_vector_oracle.json"
    )
    set_tests_properties(pyqtgraph_cpp.core.Vector PROPERTIES LABELS P2.01)
endif()

if(_pyqtgraph_cpp_has_mkcolor)
    pyqtgraph_cpp_add_cpp_test(pyqtgraph_cpp_core_mkcolor core/test_mkColor.cpp pyqtgraph_cpp.core.mkColor)
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_core_functions_p2_04
        core/test_functions_P2_04.cpp
        pyqtgraph_cpp.core.functions_P2_04
    )
    target_compile_definitions(pyqtgraph_cpp_core_functions_p2_04
        PRIVATE
            PYQTGRAPH_CPP_P2_04_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P2_04/functions_symbol_oracle.json"
    )
    set_tests_properties(pyqtgraph_cpp.core.functions_P2_04 PROPERTIES LABELS P2.04)
    pyqtgraph_cpp_add_cpp_test(pyqtgraph_cpp_core_makeqimage core/test_makeQImage.cpp pyqtgraph_cpp.core.makeQImage)
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_core_functions_qimage
        core/test_functions_qimage.cpp
        pyqtgraph_cpp.core.functions_qimage
    )
    set_tests_properties(pyqtgraph_cpp.core.functions_qimage PROPERTIES LABELS P2.11)
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_core_image_levels_qimage
        core/test_image_levels_qimage.cpp
        pyqtgraph_cpp.core.image_levels_qimage
    )
    set_tests_properties(pyqtgraph_cpp.core.image_levels_qimage PROPERTIES LABELS P2.12)
    pyqtgraph_cpp_add_cpp_test(pyqtgraph_cpp_core_colormap core/test_ColorMap.cpp pyqtgraph_cpp.core.ColorMap)
    pyqtgraph_cpp_add_cpp_test(pyqtgraph_cpp_core_colormap_lut core/test_ColorMap_LUT.cpp pyqtgraph_cpp.core.ColorMap.LUT)
    pyqtgraph_cpp_add_cpp_test(pyqtgraph_cpp_core_colormap_p2_06 core/test_ColorMap_P2_06.cpp pyqtgraph_cpp.core.ColorMap.P2_06)
    target_compile_definitions(pyqtgraph_cpp_core_colormap_p2_06
        PRIVATE
            PYQTGRAPH_CPP_P2_06_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P2_06/colormap_oracle.json"
            PYQTGRAPH_CPP_P2_06_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P2.06"
    )
    set_tests_properties(pyqtgraph_cpp.core.ColorMap.P2_06 PROPERTIES LABELS P2.06)
    pyqtgraph_cpp_add_cpp_test(pyqtgraph_cpp_core_mkpen_mkbrush core/test_mkPen_mkBrush.cpp pyqtgraph_cpp.core.mkPen_mkBrush)
endif()

if(_pyqtgraph_cpp_has_graphicsitem)
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsitems_graphicsitem
        graphicsItems/test_GraphicsItem.cpp
        pyqtgraph_cpp.graphicsItems.GraphicsItem
    )
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsitems_graphicsobject
        graphicsItems/test_GraphicsObject.cpp
        pyqtgraph_cpp.graphicsItems.GraphicsObject
    )
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsitems_graphicswidget
        graphicsItems/test_GraphicsWidget.cpp
        pyqtgraph_cpp.graphicsItems.GraphicsWidget
    )
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsitems_graphicswidgetanchor
        graphicsItems/test_GraphicsWidgetAnchor.cpp
        pyqtgraph_cpp.graphicsItems.GraphicsWidgetAnchor
    )
    set_tests_properties(
        pyqtgraph_cpp.graphicsItems.GraphicsItem
        pyqtgraph_cpp.graphicsItems.GraphicsObject
        pyqtgraph_cpp.graphicsItems.GraphicsWidget
        pyqtgraph_cpp.graphicsItems.GraphicsWidgetAnchor
        PROPERTIES LABELS P3.02
    )

    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsscene_graphicsscene
        graphicsItems/test_GraphicsScene.cpp
        pyqtgraph_cpp.GraphicsScene.GraphicsScene
    )
    set_target_properties(pyqtgraph_cpp_graphicsscene_graphicsscene PROPERTIES AUTOMOC ON)

    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsscene_event_dispatch
        graphicsItems/test_GraphicsScene_event_dispatch.cpp
        pyqtgraph_cpp.GraphicsScene.event_dispatch
    )
    set_target_properties(pyqtgraph_cpp_graphicsscene_event_dispatch PROPERTIES AUTOMOC ON)
    target_compile_definitions(pyqtgraph_cpp_graphicsscene_event_dispatch
        PRIVATE
            PYQTGRAPH_CPP_P3_03_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P3.03"
    )
    set_tests_properties(pyqtgraph_cpp.GraphicsScene.event_dispatch PROPERTIES LABELS P3.03)

    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsscene_mouseevents
        graphicsItems/test_mouseEvents.cpp
        pyqtgraph_cpp.GraphicsScene.mouseEvents
    )
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsscene_mouseevents_p3_04
        graphicsItems/test_mouseEvents_P3_04.cpp
        pyqtgraph_cpp.GraphicsScene.mouseEvents.P3_04
    )
    target_compile_definitions(pyqtgraph_cpp_graphicsscene_mouseevents_p3_04
        PRIVATE
            PYQTGRAPH_CPP_P3_04_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P3.04"
    )
    set_tests_properties(pyqtgraph_cpp.GraphicsScene.mouseEvents.P3_04 PROPERTIES LABELS P3.04)
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsitems_axisitem
        graphicsItems/test_AxisItem.cpp
        pyqtgraph_cpp.graphicsItems.AxisItem
    )
    target_compile_definitions(pyqtgraph_cpp_graphicsitems_axisitem
        PRIVATE
            PYQTGRAPH_CPP_P3_07_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual/P3.07/AxisItem"
            PYQTGRAPH_CPP_P3_07_CANONICAL_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual-diffs/P3.07-AxisItem"
            PYQTGRAPH_CPP_P3_07_GPT_REVIEW_REPORT="${PROJECT_SOURCE_DIR}/reports/visual/P3.07/AxisItem/gpt5_vision_review.md"
    )
    set_tests_properties(pyqtgraph_cpp.graphicsItems.AxisItem PROPERTIES LABELS "visual;P3.07")
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsitems_viewbox
        graphicsItems/test_ViewBox.cpp
        pyqtgraph_cpp.graphicsItems.ViewBox
    )
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsitems_viewbox_range
        graphicsItems/test_ViewBox_range.cpp
        pyqtgraph_cpp.graphicsItems.ViewBox.range
    )
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsitems_viewbox_pan_zoom
        graphicsItems/test_ViewBox_pan_zoom.cpp
        pyqtgraph_cpp.graphicsItems.ViewBox.pan_zoom
    )
    target_compile_definitions(pyqtgraph_cpp_graphicsitems_viewbox_pan_zoom
        PRIVATE
            PYQTGRAPH_CPP_INTERACTION_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/oracle/fixtures/interactions"
    )
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsitems_viewbox_transforms_autorange
        graphicsItems/test_ViewBox_transforms_autorange.cpp
        pyqtgraph_cpp.graphicsItems.ViewBox.transforms_autorange
    )
    set_tests_properties(pyqtgraph_cpp.graphicsItems.ViewBox.transforms_autorange PROPERTIES LABELS P3.05)
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsitems_viewbox_interaction_linking
        graphicsItems/test_ViewBox_interaction_linking.cpp
        pyqtgraph_cpp.graphicsItems.ViewBox.interaction_linking
    )
    target_compile_definitions(pyqtgraph_cpp_graphicsitems_viewbox_interaction_linking
        PRIVATE
            PYQTGRAPH_CPP_P3_06_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P3.06"
    )
    target_link_libraries(pyqtgraph_cpp_graphicsitems_viewbox_interaction_linking PRIVATE Qt6::Test)
    set_tests_properties(pyqtgraph_cpp.graphicsItems.ViewBox.interaction_linking PROPERTIES LABELS P3.06)

    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsitems_linearregion_infline_target_p4_09
        graphicsItems/test_LinearRegion_InfiniteLine_TargetItem_P4_09.cpp
        pyqtgraph_cpp.graphicsItems.LinearRegion_InfiniteLine_TargetItem.P4_09
    )
    target_compile_definitions(pyqtgraph_cpp_graphicsitems_linearregion_infline_target_p4_09
        PRIVATE
            PYQTGRAPH_CPP_P4_09_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P4.09"
    )
    set_tests_properties(pyqtgraph_cpp.graphicsItems.LinearRegion_InfiniteLine_TargetItem.P4_09 PROPERTIES LABELS P4.09)
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsitems_plotitem
        graphicsItems/test_PlotItem.cpp
        pyqtgraph_cpp.graphicsItems.PlotItem
    )
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_widgets_plotwidget
        widgets/test_PlotWidget.cpp
        pyqtgraph_cpp.widgets.PlotWidget
    )
    set_tests_properties(pyqtgraph_cpp.widgets.PlotWidget PROPERTIES LABELS P3.01)
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsitems_plotcurveitem
        graphicsItems/test_PlotCurveItem.cpp
        pyqtgraph_cpp.graphicsItems.PlotCurveItem
    )
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsitems_plotcurveitem_setdata
        graphicsItems/test_PlotCurveItem_setData.cpp
        pyqtgraph_cpp.graphicsItems.PlotCurveItem.setData
    )
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsitems_plotdataitem
        graphicsItems/test_PlotDataItem.cpp
        pyqtgraph_cpp.graphicsItems.PlotDataItem
    )
    set_tests_properties(pyqtgraph_cpp.graphicsItems.PlotDataItem PROPERTIES LABELS P3.10)

    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsitems_imageitem_render
        graphicsItems/test_ImageItem_render.cpp
        pyqtgraph_cpp.graphicsItems.ImageItem.render
    )
    target_compile_definitions(pyqtgraph_cpp_graphicsitems_imageitem_render
        PRIVATE
            PYQTGRAPH_CPP_P2_13_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual-diffs/ImageItem/P2.13"
    )
    set_tests_properties(pyqtgraph_cpp.graphicsItems.ImageItem.render PROPERTIES LABELS "visual;P2.13")

    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_graphicsitems_imageitem_nonuniform_p4_05
        graphicsItems/test_ImageItem_NonUniform_P4_05.cpp
        pyqtgraph_cpp.graphicsItems.ImageItem_NonUniform.P4_05
    )
    target_compile_definitions(pyqtgraph_cpp_graphicsitems_imageitem_nonuniform_p4_05
        PRIVATE
            PYQTGRAPH_CPP_P4_05_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P4.05"
    )
    set_tests_properties(pyqtgraph_cpp.graphicsItems.ImageItem_NonUniform.P4_05 PROPERTIES LABELS P4.05)

    add_executable(pyqtgraph_cpp_examples_simpleplot "${PROJECT_SOURCE_DIR}/examples/SimplePlot.cpp")
    pyqtgraph_cpp_configure_cpp_test_target(pyqtgraph_cpp_examples_simpleplot)
    add_executable(pyqtgraph_cpp_examples_imageitem "${PROJECT_SOURCE_DIR}/examples/ImageItem.cpp")
    pyqtgraph_cpp_configure_cpp_test_target(pyqtgraph_cpp_examples_imageitem)

    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_visual_plotcurveitem_bounds
        visual/P1_08_plotcurveitem_bounds.cpp
        P1.08.visual.PlotCurveItem.bounds
    )
    set_tests_properties(P1.08.visual.PlotCurveItem.bounds PROPERTIES LABELS "visual;P1.08")

    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_visual_plotcurveitem_p3_09
        visual/P3_09_plotcurveitem_paint.cpp
        P3.09.visual.PlotCurveItem.paint
    )
    target_compile_definitions(pyqtgraph_cpp_visual_plotcurveitem_p3_09
        PRIVATE
            PYQTGRAPH_CPP_P3_09_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual/P3.09"
            PYQTGRAPH_CPP_P3_09_CANONICAL_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual-diffs/P3.09-PlotCurveItem"
    )
    set_tests_properties(P3.09.visual.PlotCurveItem.paint PROPERTIES LABELS "visual;P3.09")

    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_visual_scatterplotitem_p4_01
        visual/P4_01_scatterplotitem_symbols.cpp
        P4.01.visual.ScatterPlotItem.symbol_atlas
    )
    target_compile_definitions(pyqtgraph_cpp_visual_scatterplotitem_p4_01
        PRIVATE
            PYQTGRAPH_CPP_P4_01_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual/P4.01"
            PYQTGRAPH_CPP_P4_01_CANONICAL_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual-diffs/P4.01-ScatterPlotItem"
    )
    set_tests_properties(P4.01.visual.ScatterPlotItem.symbol_atlas PROPERTIES LABELS "visual;P4.01")

    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_visual_bar_error_fillbetween_p4_02
        visual/P4_02_bar_error_fillbetween.cpp
        P4.02.visual.BarGraph_ErrorBar_FillBetween
    )
    target_compile_definitions(pyqtgraph_cpp_visual_bar_error_fillbetween_p4_02
        PRIVATE
            PYQTGRAPH_CPP_P4_02_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual/P4.02"
            PYQTGRAPH_CPP_P4_02_CANONICAL_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual-diffs/P4.02-bar-error-fillbetween"
            PYQTGRAPH_CPP_P4_02_REPOSITORY_ARTIFACT_DIR="${PROJECT_SOURCE_DIR}/reports/visual/P4.02"
            PYQTGRAPH_CPP_P4_02_REPOSITORY_CANONICAL_ARTIFACT_DIR="${PROJECT_SOURCE_DIR}/reports/visual-diffs/P4.02-bar-error-fillbetween"
    )
    set_tests_properties(P4.02.visual.BarGraph_ErrorBar_FillBetween PROPERTIES LABELS "visual;P4.02")

    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_visual_legend_grid_vtick_p4_11
        visual/P4_11_legend_grid_vtick.cpp
        P4.11.visual.Legend_Grid_VTickGroup
    )
    target_compile_definitions(pyqtgraph_cpp_visual_legend_grid_vtick_p4_11
        PRIVATE
            PYQTGRAPH_CPP_P4_11_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual/P4.11"
            PYQTGRAPH_CPP_P4_11_CANONICAL_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual-diffs/P4.11-legend-grid-vtick"
    )
    set_tests_properties(P4.11.visual.Legend_Grid_VTickGroup PROPERTIES LABELS "visual;P4.11")

    add_executable(pyqtgraph_cpp_visual_plotitem_p3_11 visual/P3_11_plotitem_layout_visual.cpp)
    pyqtgraph_cpp_configure_cpp_test_target(pyqtgraph_cpp_visual_plotitem_p3_11)
    add_test(
        NAME P3.11.visual.PlotItem.layout_axes_legend
        COMMAND "${Python3_EXECUTABLE}" -m pytest -q
            "${PROJECT_SOURCE_DIR}/tests/visual/test_P3_11_plotitem_layout_visual.py"
    )
    set_tests_properties(
        P3.11.visual.PlotItem.layout_axes_legend
        PROPERTIES
            LABELS "visual;P3.11"
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;PG_CPP_P3_11_VISUAL_RENDERER=$<TARGET_FILE:pyqtgraph_cpp_visual_plotitem_p3_11>;PG_P3_11_VISUAL_REPORTS_ROOT=${PROJECT_BINARY_DIR}/reports/visual/P3.11"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )

    add_executable(pyqtgraph_cpp_visual_render_example visual/P1_08_render_cpp_example.cpp)
    pyqtgraph_cpp_configure_cpp_test_target(pyqtgraph_cpp_visual_render_example)
    add_test(
        NAME P1.08.visual.SimplePlot
        COMMAND "${Python3_EXECUTABLE}" -m pytest -q
            "${PROJECT_SOURCE_DIR}/tests/visual/test_P1_08_cpp_visual_renderer.py"
    )
    set_tests_properties(
        P1.08.visual.SimplePlot
        PROPERTIES
            LABELS "visual;P1.08"
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;PG_CPP_VISUAL_RENDERER=$<TARGET_FILE:pyqtgraph_cpp_visual_render_example>;PG_VISUAL_REPORTS_ROOT=${PROJECT_BINARY_DIR}/reports/visual/P1.08;PG_VISUAL_REVIEW_REPORT=${PROJECT_SOURCE_DIR}/reports/visual/P1.08/SimplePlot/gpt5_vision_review.md"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
    add_test(
        NAME P3.13.visual.SimplePlot
        COMMAND "${Python3_EXECUTABLE}" -m pytest -q
            "${PROJECT_SOURCE_DIR}/tests/examples/test_SimplePlot_P3_13_visual.py"
    )
    set_tests_properties(
        P3.13.visual.SimplePlot
        PROPERTIES
            LABELS "visual;P3.13"
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;PG_CPP_P3_13_VISUAL_RENDERER=$<TARGET_FILE:pyqtgraph_cpp_visual_render_example>;PG_P3_13_VISUAL_REPORTS_ROOT=${PROJECT_BINARY_DIR}/reports/visual-diffs;PG_P3_13_EXAMPLES_REPORTS_ROOT=${PROJECT_BINARY_DIR}/reports/examples/P3.13;PG_P3_13_VISUAL_REVIEW_REPORT=${PROJECT_SOURCE_DIR}/reports/examples/P3.13/gpt5_vision_review.md"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )

    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_examples_simpleplot_test
        examples/test_SimplePlot.cpp
        pyqtgraph_cpp.examples.SimplePlot
    )
    pyqtgraph_cpp_add_cpp_test(
        pyqtgraph_cpp_examples_imageitem_test
        examples/test_ImageItem.cpp
        pyqtgraph_cpp.examples.ImageItem
    )
    target_compile_definitions(pyqtgraph_cpp_examples_imageitem_test
        PRIVATE
            PYQTGRAPH_CPP_P2_14_VISUAL_DIFF_DIR="${PROJECT_BINARY_DIR}/reports/visual-diffs/ImageItem-example"
    )
    set_tests_properties(pyqtgraph_cpp.examples.ImageItem PROPERTIES LABELS "visual;P2.14")
    pyqtgraph_cpp_add_cpp_test(pyqtgraph_cpp_hierarchy_cpp hierarchy/test_cpp_hierarchy.cpp pyqtgraph_cpp.hierarchy.cpp)
endif()

add_test(
    NAME P0.08.performance.schema.create_baseline
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/run_performance"
        --mode create-baseline
        --baseline "${PROJECT_BINARY_DIR}/reports/performance/P0.08/create-baseline.json"
        --output "${PROJECT_BINARY_DIR}/reports/performance/P0.08/create-metrics.json"
)
add_test(
    NAME P0.08.performance.schema.update_baseline
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/run_performance"
        --mode update-baseline
        --baseline "${PROJECT_BINARY_DIR}/reports/performance/P0.08/update-baseline.json"
        --output "${PROJECT_BINARY_DIR}/reports/performance/P0.08/update-metrics.json"
        --rationale "P0.08 focused proof exercises explicit baseline update mode."
)
add_test(
    NAME P0.08.performance.schema
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/run_performance"
        --mode compare
        --baseline "${PROJECT_SOURCE_DIR}/reports/performance/P0.08/baseline.json"
        --output "${PROJECT_BINARY_DIR}/reports/performance/P0.08/metrics.json"
        --synthetic-regression-check
)
set_tests_properties(
    P0.08.performance.schema.create_baseline
    P0.08.performance.schema.update_baseline
    P0.08.performance.schema
    PROPERTIES LABELS "performance;P0.08"
)
