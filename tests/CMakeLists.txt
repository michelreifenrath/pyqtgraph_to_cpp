function(cppqtgraph_configure_cpp_test_target target)
    target_link_libraries(${target}
        PRIVATE
            cppqtgraph
            cppqtgraph_project_options
            cppqtgraph_project_warnings
    )
    cppqtgraph_enable_sanitizers(${target})
endfunction()

function(cppqtgraph_add_cpp_test target source test_name)
    add_executable(${target} ${source})
    cppqtgraph_configure_cpp_test_target(${target})
    add_test(NAME ${test_name} COMMAND ${target})
endfunction()

add_executable(cppqtgraph_smoke_empty smoke/test_empty.cpp)
target_link_libraries(cppqtgraph_smoke_empty
    PRIVATE
        cppqtgraph_project_options
        cppqtgraph_project_warnings
)
cppqtgraph_enable_sanitizers(cppqtgraph_smoke_empty)

add_test(NAME cppqtgraph.smoke.empty COMMAND cppqtgraph_smoke_empty)

if(NOT CPPQTGRAPH_HAS_QT)
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

add_executable(cppqtgraph_oracle_P0_06 oracle/P0_06_oracle_comparison.cpp)
target_link_libraries(cppqtgraph_oracle_P0_06
    PRIVATE
        cppqtgraph_project_options
        cppqtgraph_project_warnings
)
target_compile_definitions(cppqtgraph_oracle_P0_06
    PRIVATE
        CPPQTGRAPH_P0_06_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P0_06/probe_contract.json"
)
cppqtgraph_enable_sanitizers(cppqtgraph_oracle_P0_06)
add_test(NAME cppqtgraph.oracle.P0_06 COMMAND cppqtgraph_oracle_P0_06)
set_tests_properties(cppqtgraph.oracle.P0_06 PROPERTIES LABELS P0.06)
add_test(NAME cppqtgraph.oracle.P0_06.mismatch COMMAND cppqtgraph_oracle_P0_06 --mismatch)
set_tests_properties(cppqtgraph.oracle.P0_06.mismatch PROPERTIES LABELS P0.06 WILL_FAIL TRUE)

add_test(
    NAME cppqtgraph.oracle.P2_01
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/oracle/probes/P2_01_point_vector_oracle.py" --check
)
set_tests_properties(
    cppqtgraph.oracle.P2_01
    PROPERTIES
        LABELS P2.01
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_test(
    NAME cppqtgraph.oracle.P2_08
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/oracle/scripts/generate_P2_08_signal_proxy_timer_oracle.py" --check
)
set_tests_properties(
    cppqtgraph.oracle.P2_08
    PROPERTIES
        LABELS P2.08
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_test(
    NAME cppqtgraph.oracle.P2_09
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/oracle/scripts/generate_P2_09_widgetgroup_oracle.py" --check
)
set_tests_properties(
    cppqtgraph.oracle.P2_09
    PROPERTIES
        LABELS P2.09
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_test(
    NAME cppqtgraph.oracle.P3_08.DateAxisItem
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/oracle/probes/P3_08_dateaxis_oracle.py" --check
)
set_tests_properties(
    cppqtgraph.oracle.P3_08.DateAxisItem
    PROPERTIES
        LABELS P3.08
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_test(
    NAME cppqtgraph.oracle.P4_04.Boxplot_PColorMesh
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/oracle/probes/P4_04_boxplot_pcolormesh_oracle.py" --check
)
set_tests_properties(
    cppqtgraph.oracle.P4_04.Boxplot_PColorMesh
    PROPERTIES
        LABELS P4.04
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_test(
    NAME cppqtgraph.oracle.P4_07.IsocurveItem
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/oracle/probes/P4_07_isocurve_oracle.py" --check
)
set_tests_properties(
    cppqtgraph.oracle.P4_07.IsocurveItem
    PROPERTIES
        LABELS P4.07
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_test(
    NAME cppqtgraph.oracle.P2_03
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/oracle/probes/P2_03_plotdata_oracle.py" --check
)
set_tests_properties(
    cppqtgraph.oracle.P2_03
    PROPERTIES
        LABELS P2.03
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_test(
    NAME cppqtgraph.oracle.P2_02
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/oracle/probes/P2_02_transform_srt_oracle.py" --check
)
set_tests_properties(
    cppqtgraph.oracle.P2_02
    PROPERTIES
        LABELS P2.02
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_test(
    NAME cppqtgraph.oracle.P2_04
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/oracle/probes/P2_04_functions_symbol_oracle.py" --check
)
set_tests_properties(
    cppqtgraph.oracle.P2_04
    PROPERTIES
        LABELS P2.04
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_test(
    NAME cppqtgraph.oracle.P2_06
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/oracle/probes/P2_06_colormap_oracle.py" --check
)
set_tests_properties(
    cppqtgraph.oracle.P2_06
    PROPERTIES
        LABELS P2.06
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_test(
    NAME cppqtgraph.oracle.P2_07
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/oracle/probes/P2_07_core_helpers_oracle.py" --check
)
set_tests_properties(
    cppqtgraph.oracle.P2_07
    PROPERTIES
        LABELS P2.07
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

cppqtgraph_add_cpp_test(cppqtgraph_core_arrayview core/test_ArrayView.cpp cppqtgraph.core.ArrayView)
cppqtgraph_add_cpp_test(cppqtgraph_core_nan_minmax core/test_nan_minmax.cpp cppqtgraph.core.nan_minmax)
cppqtgraph_add_cpp_test(cppqtgraph_core_plotdata core/test_PlotData.cpp cppqtgraph.core.PlotData)
target_compile_definitions(cppqtgraph_core_plotdata
    PRIVATE
        CPPQTGRAPH_P2_03_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P2_03/plotdata_oracle.json"
)
set_tests_properties(cppqtgraph.core.PlotData PROPERTIES LABELS P2.03)

cppqtgraph_add_cpp_test(
    cppqtgraph_core_config_debug_exception_units_p2_07
    core/test_config_debug_exception_units_P2_07.cpp
    cppqtgraph.core.config_debug_exception_units_P2_07
)
target_compile_definitions(cppqtgraph_core_config_debug_exception_units_p2_07
    PRIVATE
        CPPQTGRAPH_P2_07_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P2_07/core_helpers_oracle.json"
)
set_tests_properties(cppqtgraph.core.config_debug_exception_units_P2_07 PROPERTIES LABELS P2.07)

if(_cppqtgraph_has_qtcore)
    cppqtgraph_add_cpp_test(cppqtgraph_core_point core/test_Point.cpp cppqtgraph.core.Point)
    target_compile_definitions(cppqtgraph_core_point
        PRIVATE
            CPPQTGRAPH_P2_01_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P2_01/point_vector_oracle.json"
    )
    set_tests_properties(cppqtgraph.core.Point PROPERTIES LABELS P2.01)

    cppqtgraph_add_cpp_test(
        cppqtgraph_core_signalproxy_threadsafe_timer
        core/test_SignalProxy_ThreadsafeTimer.cpp
        cppqtgraph.core.SignalProxy_ThreadsafeTimer
    )
    target_compile_definitions(cppqtgraph_core_signalproxy_threadsafe_timer
        PRIVATE
            CPPQTGRAPH_P2_08_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P2_08/signal_proxy_timer_oracle.json"
    )
    set_tests_properties(cppqtgraph.core.SignalProxy_ThreadsafeTimer PROPERTIES LABELS P2.08)
endif()

if(_cppqtgraph_has_graphicsitem)
    cppqtgraph_add_cpp_test(
        cppqtgraph_core_widgetgroup_p2_09
        core/test_WidgetGroup_P2_09.cpp
        cppqtgraph.core.WidgetGroup_P2_09
    )
    target_compile_definitions(cppqtgraph_core_widgetgroup_p2_09
        PRIVATE
            CPPQTGRAPH_P2_09_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P2_09/widgetgroup_oracle.json"
    )
    set_tests_properties(cppqtgraph.core.WidgetGroup_P2_09 PROPERTIES LABELS P2.09)
endif()

if(_cppqtgraph_has_vector)
    cppqtgraph_add_cpp_test(cppqtgraph_core_vector core/test_Vector.cpp cppqtgraph.core.Vector)
    target_compile_definitions(cppqtgraph_core_vector
        PRIVATE
            CPPQTGRAPH_P2_01_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P2_01/point_vector_oracle.json"
    )
    set_tests_properties(cppqtgraph.core.Vector PROPERTIES LABELS P2.01)

    cppqtgraph_add_cpp_test(
        cppqtgraph_core_transform3d_srttransform
        core/test_Transform3D_SRTTransform.cpp
        cppqtgraph.core.Transform3D_SRTTransform
    )
    target_compile_definitions(cppqtgraph_core_transform3d_srttransform
        PRIVATE
            CPPQTGRAPH_P2_02_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P2_02/transform_srt_oracle.json"
    )
    set_tests_properties(cppqtgraph.core.Transform3D_SRTTransform PROPERTIES LABELS P2.02)
endif()

if(_cppqtgraph_has_mkcolor)
    cppqtgraph_add_cpp_test(cppqtgraph_core_mkcolor core/test_mkColor.cpp cppqtgraph.core.mkColor)
    cppqtgraph_add_cpp_test(
        cppqtgraph_core_functions_p2_04
        core/test_functions_P2_04.cpp
        cppqtgraph.core.functions_P2_04
    )
    target_compile_definitions(cppqtgraph_core_functions_p2_04
        PRIVATE
            CPPQTGRAPH_P2_04_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P2_04/functions_symbol_oracle.json"
    )
    set_tests_properties(cppqtgraph.core.functions_P2_04 PROPERTIES LABELS P2.04)
    cppqtgraph_add_cpp_test(cppqtgraph_core_makeqimage core/test_makeQImage.cpp cppqtgraph.core.makeQImage)
    cppqtgraph_add_cpp_test(
        cppqtgraph_core_functions_qimage
        core/test_functions_qimage.cpp
        cppqtgraph.core.functions_qimage
    )
    set_tests_properties(cppqtgraph.core.functions_qimage PROPERTIES LABELS P2.11)
    cppqtgraph_add_cpp_test(
        cppqtgraph_core_image_levels_qimage
        core/test_image_levels_qimage.cpp
        cppqtgraph.core.image_levels_qimage
    )
    set_tests_properties(cppqtgraph.core.image_levels_qimage PROPERTIES LABELS P2.12)
    cppqtgraph_add_cpp_test(cppqtgraph_core_colormap core/test_ColorMap.cpp cppqtgraph.core.ColorMap)
    cppqtgraph_add_cpp_test(cppqtgraph_core_colormap_lut core/test_ColorMap_LUT.cpp cppqtgraph.core.ColorMap.LUT)
    cppqtgraph_add_cpp_test(cppqtgraph_core_colormap_p2_06 core/test_ColorMap_P2_06.cpp cppqtgraph.core.ColorMap.P2_06)
    target_compile_definitions(cppqtgraph_core_colormap_p2_06
        PRIVATE
            CPPQTGRAPH_P2_06_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P2_06/colormap_oracle.json"
            CPPQTGRAPH_P2_06_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P2.06"
    )
    set_tests_properties(cppqtgraph.core.ColorMap.P2_06 PROPERTIES LABELS P2.06)
    cppqtgraph_add_cpp_test(cppqtgraph_core_mkpen_mkbrush core/test_mkPen_mkBrush.cpp cppqtgraph.core.mkPen_mkBrush)
endif()

if(_cppqtgraph_has_graphicsitem)
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_graphicsitem
        graphicsItems/test_GraphicsItem.cpp
        cppqtgraph.graphicsItems.GraphicsItem
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_graphicsobject
        graphicsItems/test_GraphicsObject.cpp
        cppqtgraph.graphicsItems.GraphicsObject
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_graphicswidget
        graphicsItems/test_GraphicsWidget.cpp
        cppqtgraph.graphicsItems.GraphicsWidget
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_graphicswidgetanchor
        graphicsItems/test_GraphicsWidgetAnchor.cpp
        cppqtgraph.graphicsItems.GraphicsWidgetAnchor
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_uihelpers_p4_13
        graphicsItems/test_UIHelpers_P4_13.cpp
        cppqtgraph.graphicsItems.UIHelpers.P4_13
    )
    set_tests_properties(
        cppqtgraph.graphicsItems.GraphicsItem
        cppqtgraph.graphicsItems.GraphicsObject
        cppqtgraph.graphicsItems.GraphicsWidget
        cppqtgraph.graphicsItems.GraphicsWidgetAnchor
        PROPERTIES LABELS P3.02
    )
    set_tests_properties(cppqtgraph.graphicsItems.UIHelpers.P4_13 PROPERTIES LABELS P4.13)

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsscene_graphicsscene
        graphicsItems/test_GraphicsScene.cpp
        cppqtgraph.GraphicsScene.GraphicsScene
    )
    set_target_properties(cppqtgraph_graphicsscene_graphicsscene PROPERTIES AUTOMOC ON)

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsscene_event_dispatch
        graphicsItems/test_GraphicsScene_event_dispatch.cpp
        cppqtgraph.GraphicsScene.event_dispatch
    )
    set_target_properties(cppqtgraph_graphicsscene_event_dispatch PROPERTIES AUTOMOC ON)
    target_compile_definitions(cppqtgraph_graphicsscene_event_dispatch
        PRIVATE
            CPPQTGRAPH_P3_03_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P3.03"
    )
    set_tests_properties(cppqtgraph.GraphicsScene.event_dispatch PROPERTIES LABELS P3.03)

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsscene_mouseevents
        graphicsItems/test_mouseEvents.cpp
        cppqtgraph.GraphicsScene.mouseEvents
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsscene_mouseevents_p3_04
        graphicsItems/test_mouseEvents_P3_04.cpp
        cppqtgraph.GraphicsScene.mouseEvents.P3_04
    )
    target_compile_definitions(cppqtgraph_graphicsscene_mouseevents_p3_04
        PRIVATE
            CPPQTGRAPH_P3_04_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P3.04"
    )
    set_tests_properties(cppqtgraph.GraphicsScene.mouseEvents.P3_04 PROPERTIES LABELS P3.04)
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_axisitem
        graphicsItems/test_AxisItem.cpp
        cppqtgraph.graphicsItems.AxisItem
    )
    target_compile_definitions(cppqtgraph_graphicsitems_axisitem
        PRIVATE
            CPPQTGRAPH_P3_07_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual/P3.07/AxisItem"
            CPPQTGRAPH_P3_07_CANONICAL_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual-diffs/P3.07-AxisItem"
            CPPQTGRAPH_P3_07_GPT_REVIEW_REPORT="${PROJECT_SOURCE_DIR}/reports/visual/P3.07/AxisItem/gpt5_vision_review.md"
    )
    set_tests_properties(cppqtgraph.graphicsItems.AxisItem PROPERTIES LABELS "visual;P3.07")
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_axisitem_p5_parity
        graphicsItems/test_AxisItem_p5_parity.cpp
        cppqtgraph.graphicsItems.AxisItem.p5_parity
    )
    set_tests_properties(cppqtgraph.graphicsItems.AxisItem.p5_parity PROPERTIES LABELS "P388")
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_axisitem_interaction
        graphicsItems/test_AxisItem_interaction.cpp
        cppqtgraph.graphicsItems.AxisItem.interaction
    )
    set_tests_properties(cppqtgraph.graphicsItems.AxisItem.interaction PROPERTIES LABELS "P389")
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_dateaxisitem_p3_08
        graphicsItems/test_DateAxisItem_P3_08.cpp
        cppqtgraph.graphicsItems.DateAxisItem.P3_08
    )
    target_compile_definitions(cppqtgraph_graphicsitems_dateaxisitem_p3_08
        PRIVATE
            CPPQTGRAPH_P3_08_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual-diffs/P3.08-DateAxisItem"
    )
    set_tests_properties(cppqtgraph.graphicsItems.DateAxisItem.P3_08 PROPERTIES LABELS "visual;P3.08")
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_viewbox
        graphicsItems/test_ViewBox.cpp
        cppqtgraph.graphicsItems.ViewBox
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_viewbox_range
        graphicsItems/test_ViewBox_range.cpp
        cppqtgraph.graphicsItems.ViewBox.range
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_viewbox_pan_zoom
        graphicsItems/test_ViewBox_pan_zoom.cpp
        cppqtgraph.graphicsItems.ViewBox.pan_zoom
    )
    target_compile_definitions(cppqtgraph_graphicsitems_viewbox_pan_zoom
        PRIVATE
            CPPQTGRAPH_INTERACTION_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/oracle/fixtures/interactions"
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_viewbox_transforms_autorange
        graphicsItems/test_ViewBox_transforms_autorange.cpp
        cppqtgraph.graphicsItems.ViewBox.transforms_autorange
    )
    set_tests_properties(cppqtgraph.graphicsItems.ViewBox.transforms_autorange PROPERTIES LABELS P3.05)
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_viewbox_interaction_robustness
        graphicsItems/test_ViewBox_interaction_robustness.cpp
        cppqtgraph.graphicsItems.ViewBox.interaction_robustness
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_viewbox_interaction_linking
        graphicsItems/test_ViewBox_interaction_linking.cpp
        cppqtgraph.graphicsItems.ViewBox.interaction_linking
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_viewbox_context_menu
        graphicsItems/test_ViewBox_context_menu.cpp
        cppqtgraph.graphicsItems.ViewBox.context_menu
    )
    target_compile_definitions(cppqtgraph_graphicsitems_viewbox_interaction_linking
        PRIVATE
            CPPQTGRAPH_P3_06_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P3.06"
    )
    target_link_libraries(cppqtgraph_graphicsitems_viewbox_interaction_linking PRIVATE Qt6::Test)
    set_tests_properties(cppqtgraph.graphicsItems.ViewBox.interaction_linking PROPERTIES LABELS P3.06)

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_linearregion_infline_target_p4_09
        graphicsItems/test_LinearRegion_InfiniteLine_TargetItem_P4_09.cpp
        cppqtgraph.graphicsItems.LinearRegion_InfiniteLine_TargetItem.P4_09
    )
    target_compile_definitions(cppqtgraph_graphicsitems_linearregion_infline_target_p4_09
        PRIVATE
            CPPQTGRAPH_P4_09_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P4.09"
    )
    set_tests_properties(cppqtgraph.graphicsItems.LinearRegion_InfiniteLine_TargetItem.P4_09 PROPERTIES LABELS P4.09)

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_roi_geometry_p4_16
        graphicsItems/test_ROI_geometry_P4_16.cpp
        cppqtgraph.graphicsItems.ROI.geometry.P4_16
    )
    target_compile_definitions(cppqtgraph_graphicsitems_roi_geometry_p4_16
        PRIVATE
            CPPQTGRAPH_P4_16_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P4.16"
            CPPQTGRAPH_P4_16_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P4.16"
    )
    set_tests_properties(cppqtgraph.graphicsItems.ROI.geometry.P4_16 PROPERTIES LABELS P4.16)

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_roi_handles_p4_17
        graphicsItems/test_ROI_handles_P4_17.cpp
        cppqtgraph.graphicsItems.ROI.handles.P4_17
    )
    target_compile_definitions(cppqtgraph_graphicsitems_roi_handles_p4_17
        PRIVATE
            CPPQTGRAPH_P4_17_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P4.17"
            CPPQTGRAPH_P4_17_VISUAL_DIFF_DIR="${PROJECT_BINARY_DIR}/reports/visual-diffs/ROI-handles"
            CPPQTGRAPH_P4_17_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P4.17"
    )
    set_tests_properties(cppqtgraph.graphicsItems.ROI.handles.P4_17 PROPERTIES LABELS P4.17)

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_roi_transform_p4_18
        graphicsItems/test_ROI_transform_P4_18.cpp
        cppqtgraph.graphicsItems.ROI.transform.P4_18
    )
    target_compile_definitions(cppqtgraph_graphicsitems_roi_transform_p4_18
        PRIVATE
            CPPQTGRAPH_P4_18_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P4.18"
            CPPQTGRAPH_P4_18_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P4.18"
    )
    set_tests_properties(cppqtgraph.graphicsItems.ROI.transform.P4_18 PROPERTIES LABELS P4.18)

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_roi_image_p4_19
        graphicsItems/test_ROI_image_P4_19.cpp
        cppqtgraph.graphicsItems.ROI.image.P4_19
    )
    target_compile_definitions(cppqtgraph_graphicsitems_roi_image_p4_19
        PRIVATE
            CPPQTGRAPH_P4_19_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P4.19"
            CPPQTGRAPH_P4_19_VISUAL_DIFF_DIR="${PROJECT_BINARY_DIR}/reports/visual-diffs/ROI-image-extraction"
            CPPQTGRAPH_P4_19_GPT_REVIEW_REPORT="${PROJECT_SOURCE_DIR}/reports/issues/P4.19/gpt5_vision_review.md"
            CPPQTGRAPH_P4_19_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P4.19"
    )
    set_tests_properties(cppqtgraph.graphicsItems.ROI.image.P4_19 PROPERTIES LABELS "visual;P4.19")

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_roi_shapes_p4_20
        graphicsItems/test_ROI_shapes_P4_20.cpp
        cppqtgraph.graphicsItems.ROI.shapes.P4_20
    )
    target_compile_definitions(cppqtgraph_graphicsitems_roi_shapes_p4_20
        PRIVATE
            CPPQTGRAPH_P4_20_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P4.20"
            CPPQTGRAPH_P4_20_VISUAL_DIFF_DIR="${PROJECT_SOURCE_DIR}/reports/visual-diffs/ROI-shapes"
            CPPQTGRAPH_P4_20_GPT_REVIEW_REPORT="${PROJECT_SOURCE_DIR}/reports/visual-diffs/ROI-shapes/gpt5_vision_review.md"
            CPPQTGRAPH_P4_20_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P4.20"
    )
    set_tests_properties(cppqtgraph.graphicsItems.ROI.shapes.P4_20 PROPERTIES LABELS "visual;P4.20")

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_arrowitem_p4_21
        graphicsItems/test_ArrowItem_P4_21.cpp
        cppqtgraph.graphicsItems.ArrowItem.P4_21
    )
    target_compile_definitions(cppqtgraph_graphicsitems_arrowitem_p4_21
        PRIVATE
            CPPQTGRAPH_P4_21_VISUAL_DIFF_DIR="${PROJECT_SOURCE_DIR}/reports/visual-diffs/ArrowItem"
            CPPQTGRAPH_P4_21_GPT_REVIEW_REPORT="${PROJECT_SOURCE_DIR}/reports/visual-diffs/ArrowItem/gpt5_vision_review.md"
            CPPQTGRAPH_P4_21_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P4.21"
    )
    set_tests_properties(cppqtgraph.graphicsItems.ArrowItem.P4_21 PROPERTIES LABELS "visual;P4.21")

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_textlabelitem_p4_22
        graphicsItems/test_TextItem_LabelItem_P4_22.cpp
        cppqtgraph.graphicsItems.TextLabelItem.P4_22
    )
    target_compile_definitions(cppqtgraph_graphicsitems_textlabelitem_p4_22
        PRIVATE
            CPPQTGRAPH_P4_22_VISUAL_DIFF_DIR="${PROJECT_SOURCE_DIR}/reports/visual-diffs/TextLabelItem"
            CPPQTGRAPH_P4_22_GPT_REVIEW_REPORT="${PROJECT_SOURCE_DIR}/reports/visual-diffs/TextLabelItem/gpt5_vision_review.md"
            CPPQTGRAPH_P4_22_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P4.22"
    )
    set_tests_properties(cppqtgraph.graphicsItems.TextLabelItem.P4_22 PROPERTIES LABELS "visual;P4.22")

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_scalebar_p4_23
        graphicsItems/test_ScaleBar_P4_23.cpp
        cppqtgraph.graphicsItems.ScaleBar.P4_23
    )
    target_compile_definitions(cppqtgraph_graphicsitems_scalebar_p4_23
        PRIVATE
            CPPQTGRAPH_P4_23_VISUAL_DIFF_DIR="${PROJECT_SOURCE_DIR}/reports/visual-diffs/ScaleBar"
            CPPQTGRAPH_P4_23_GPT_REVIEW_REPORT="${PROJECT_SOURCE_DIR}/reports/visual-diffs/ScaleBar/gpt5_vision_review.md"
            CPPQTGRAPH_P4_23_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P4.23"
    )
    set_tests_properties(cppqtgraph.graphicsItems.ScaleBar.P4_23 PROPERTIES LABELS "visual;P4.23")

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_gradienteditoritem_p4_24
        graphicsItems/test_GradientEditorItem_P4_24.cpp
        cppqtgraph.graphicsItems.GradientEditorItem.P4_24
    )
    target_compile_definitions(cppqtgraph_graphicsitems_gradienteditoritem_p4_24
        PRIVATE
            CPPQTGRAPH_P4_24_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P4.24"
            CPPQTGRAPH_P4_24_VISUAL_DIFF_DIR="${PROJECT_SOURCE_DIR}/reports/visual-diffs/GradientEditorItem"
            CPPQTGRAPH_P4_24_GPT_REVIEW_REPORT="${PROJECT_SOURCE_DIR}/reports/visual-diffs/GradientEditorItem/gpt5_vision_review.md"
            CPPQTGRAPH_P4_24_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P4.24"
    )
    set_tests_properties(cppqtgraph.graphicsItems.GradientEditorItem.P4_24 PROPERTIES LABELS "visual;P4.24")

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_gradientlegend_p4_25
        graphicsItems/test_GradientLegend_P4_25.cpp
        cppqtgraph.graphicsItems.GradientLegend.P4_25
    )
    target_compile_definitions(cppqtgraph_graphicsitems_gradientlegend_p4_25
        PRIVATE
            CPPQTGRAPH_P4_25_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P4.25"
            CPPQTGRAPH_P4_25_VISUAL_DIFF_DIR="${PROJECT_SOURCE_DIR}/reports/visual-diffs/GradientLegend"
            CPPQTGRAPH_P4_25_GPT_REVIEW_REPORT="${PROJECT_SOURCE_DIR}/reports/visual-diffs/GradientLegend/gpt5_vision_review.md"
            CPPQTGRAPH_P4_25_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P4.25"
    )
    set_tests_properties(cppqtgraph.graphicsItems.GradientLegend.P4_25 PROPERTIES LABELS "visual;P4.25")

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_plotitem
        graphicsItems/test_PlotItem.cpp
        cppqtgraph.graphicsItems.PlotItem
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_plotitem_menu_config_p4_14
        graphicsItems/test_PlotItem_menu_config_P4_14.cpp
        cppqtgraph.graphicsItems.PlotItem.menu_config.P4_14
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_plotitem_autorange_button
        graphicsItems/test_PlotItem_autorange_button.cpp
        cppqtgraph.graphicsItems.PlotItem.autorange_button
    )
    target_compile_definitions(cppqtgraph_graphicsitems_plotitem_menu_config_p4_14
        PRIVATE
            CPPQTGRAPH_P4_14_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P4.14"
    )
    set_tests_properties(cppqtgraph.graphicsItems.PlotItem.menu_config.P4_14 PROPERTIES LABELS P4.14)
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_plotwidget
        widgets/test_PlotWidget.cpp
        cppqtgraph.widgets.PlotWidget
    )
    set_tests_properties(cppqtgraph.widgets.PlotWidget PROPERTIES LABELS P3.01)
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_graphicslayoutwidget_p3_12
        widgets/test_GraphicsLayoutWidget_P3_12.cpp
        cppqtgraph.widgets.GraphicsLayoutWidget.P3_12
    )
    target_compile_definitions(cppqtgraph_widgets_graphicslayoutwidget_p3_12
        PRIVATE
            CPPQTGRAPH_P3_12_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P3.12"
    )
    set_tests_properties(cppqtgraph.widgets.GraphicsLayoutWidget.P3_12 PROPERTIES LABELS P3.12)
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_plotting_widgets_p5_01
        widgets/test_plotting_widgets_P5_01.cpp
        cppqtgraph.widgets.plotting_widgets.P5_01
    )
    set_target_properties(cppqtgraph_widgets_plotting_widgets_p5_01 PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph_widgets_plotting_widgets_p5_01 PRIVATE Qt6::Test)
    target_compile_definitions(cppqtgraph_widgets_plotting_widgets_p5_01
        PRIVATE
            CPPQTGRAPH_P5_01_VISUAL_DIFF_DIR="${PROJECT_SOURCE_DIR}/reports/visual-diffs/P5.01"
            CPPQTGRAPH_P5_01_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P5.01"
    )
    set_tests_properties(cppqtgraph.widgets.plotting_widgets.P5_01 PROPERTIES LABELS "visual;P5.01")
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_colorbutton_p5_12
        widgets/test_ColorButton_P5_12.cpp
        cppqtgraph.widgets.ColorButton.P5_12
    )
    set_target_properties(cppqtgraph_widgets_colorbutton_p5_12 PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph_widgets_colorbutton_p5_12 PRIVATE Qt6::Test)
    target_compile_definitions(cppqtgraph_widgets_colorbutton_p5_12
        PRIVATE
            CPPQTGRAPH_P5_12_VISUAL_DIFF_DIR="${PROJECT_SOURCE_DIR}/reports/visual-diffs/ColorButton"
            CPPQTGRAPH_P5_12_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P5.12"
    )
    set_tests_properties(cppqtgraph.widgets.ColorButton.P5_12 PROPERTIES LABELS "visual;P5.12")
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_colormapmenu_p5_13
        widgets/test_ColorMapMenu_P5_13.cpp
        cppqtgraph.widgets.ColorMapMenu.P5_13
    )
    set_target_properties(cppqtgraph_widgets_colormapmenu_p5_13 PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph_widgets_colormapmenu_p5_13 PRIVATE Qt6::Test)
    target_compile_definitions(cppqtgraph_widgets_colormapmenu_p5_13
        PRIVATE
            CPPQTGRAPH_P5_13_VISUAL_DIFF_DIR="${PROJECT_SOURCE_DIR}/reports/visual-diffs/ColorMapMenu"
            CPPQTGRAPH_P5_13_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P5.13"
    )
    set_tests_properties(cppqtgraph.widgets.ColorMapMenu.P5_13 PROPERTIES LABELS "visual;P5.13")
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_colormapgradientwidget_p5_14
        widgets/test_ColorMapGradientWidget_P5_14.cpp
        cppqtgraph.widgets.ColorMapGradientWidget.P5_14
    )
    set_target_properties(cppqtgraph_widgets_colormapgradientwidget_p5_14 PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph_widgets_colormapgradientwidget_p5_14 PRIVATE Qt6::Test)
    target_compile_definitions(cppqtgraph_widgets_colormapgradientwidget_p5_14
        PRIVATE
            CPPQTGRAPH_P5_14_VISUAL_DIFF_DIR="${PROJECT_SOURCE_DIR}/reports/visual-diffs/ColorMapGradientWidget"
            CPPQTGRAPH_P5_14_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P5.14"
    )
    set_tests_properties(cppqtgraph.widgets.ColorMapGradientWidget.P5_14 PROPERTIES LABELS "visual;P5.14")
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_treewidget_checktable_p5_15
        widgets/test_TreeWidget_CheckTable_P5_15.cpp
        cppqtgraph.widgets.TreeWidget_CheckTable.P5_15
    )
    set_target_properties(cppqtgraph_widgets_treewidget_checktable_p5_15 PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph_widgets_treewidget_checktable_p5_15 PRIVATE Qt6::Test)
    target_compile_definitions(cppqtgraph_widgets_treewidget_checktable_p5_15
        PRIVATE
            CPPQTGRAPH_P5_15_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P5.15"
    )
    set_tests_properties(cppqtgraph.widgets.TreeWidget_CheckTable.P5_15 PROPERTIES LABELS P5.15)
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_datatreewidget_difftreewidget_p5_16
        widgets/test_DataTreeWidget_DiffTreeWidget_P5_16.cpp
        cppqtgraph.widgets.DataTreeWidget_DiffTreeWidget.P5_16
    )
    set_target_properties(cppqtgraph_widgets_datatreewidget_difftreewidget_p5_16 PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph_widgets_datatreewidget_difftreewidget_p5_16 PRIVATE Qt6::Test)
    target_compile_definitions(cppqtgraph_widgets_datatreewidget_difftreewidget_p5_16
        PRIVATE
            CPPQTGRAPH_P5_16_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P5.16"
    )
    set_tests_properties(cppqtgraph.widgets.DataTreeWidget_DiffTreeWidget.P5_16 PROPERTIES LABELS P5.16)
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_tablewidget_p5_17
        widgets/test_TableWidget_P5_17.cpp
        cppqtgraph.widgets.TableWidget.P5_17
    )
    set_target_properties(cppqtgraph_widgets_tablewidget_p5_17 PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph_widgets_tablewidget_p5_17 PRIVATE Qt6::Test)
    target_compile_definitions(cppqtgraph_widgets_tablewidget_p5_17
        PRIVATE
            CPPQTGRAPH_P5_17_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P5.17"
    )
    set_tests_properties(cppqtgraph.widgets.TableWidget.P5_17 PROPERTIES LABELS P5.17)
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_combobox_p5_18
        widgets/test_ComboBox_P5_18.cpp
        cppqtgraph.widgets.ComboBox.P5_18
    )
    set_target_properties(cppqtgraph_widgets_combobox_p5_18 PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph_widgets_combobox_p5_18 PRIVATE Qt6::Test)
    target_compile_definitions(cppqtgraph_widgets_combobox_p5_18
        PRIVATE
            CPPQTGRAPH_P5_18_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P5.18"
    )
    set_tests_properties(cppqtgraph.widgets.ComboBox.P5_18 PROPERTIES LABELS P5.18)
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_spinbox_p5_19
        widgets/test_SpinBox_P5_19.cpp
        cppqtgraph.widgets.SpinBox.P5_19
    )
    set_target_properties(cppqtgraph_widgets_spinbox_p5_19 PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph_widgets_spinbox_p5_19 PRIVATE Qt6::Test)
    target_compile_definitions(cppqtgraph_widgets_spinbox_p5_19
        PRIVATE
            CPPQTGRAPH_P5_19_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P5.19"
    )
    set_tests_properties(cppqtgraph.widgets.SpinBox.P5_19 PROPERTIES LABELS P5.19)
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_joystickbutton_verticallabel_p5_20
        widgets/test_JoystickButton_VerticalLabel_P5_20.cpp
        cppqtgraph.widgets.JoystickButton_VerticalLabel.P5_20
    )
    set_target_properties(cppqtgraph_widgets_joystickbutton_verticallabel_p5_20 PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph_widgets_joystickbutton_verticallabel_p5_20 PRIVATE Qt6::Test)
    target_compile_definitions(cppqtgraph_widgets_joystickbutton_verticallabel_p5_20
        PRIVATE
            CPPQTGRAPH_P5_20_VISUAL_DIFF_DIR="${PROJECT_SOURCE_DIR}/reports/visual-diffs/JoystickVerticalLabel"
            CPPQTGRAPH_P5_20_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P5.20"
    )
    set_tests_properties(cppqtgraph.widgets.JoystickButton_VerticalLabel.P5_20 PROPERTIES LABELS "visual;P5.20")
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_layoutwidget_p5_21
        widgets/test_LayoutWidget_P5_21.cpp
        cppqtgraph.widgets.LayoutWidget.P5_21
    )
    set_target_properties(cppqtgraph_widgets_layoutwidget_p5_21 PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph_widgets_layoutwidget_p5_21 PRIVATE Qt6::Test)
    target_compile_definitions(cppqtgraph_widgets_layoutwidget_p5_21
        PRIVATE
            CPPQTGRAPH_P5_21_VISUAL_DIFF_DIR="${PROJECT_SOURCE_DIR}/reports/visual-diffs/LayoutWidget"
            CPPQTGRAPH_P5_21_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P5.21"
    )
    set_tests_properties(cppqtgraph.widgets.LayoutWidget.P5_21 PROPERTIES LABELS "visual;P5.21")
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_groupbox_p5_22
        widgets/test_GroupBox_P5_22.cpp
        cppqtgraph.widgets.GroupBox.P5_22
    )
    set_target_properties(cppqtgraph_widgets_groupbox_p5_22 PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph_widgets_groupbox_p5_22 PRIVATE Qt6::Test)
    target_compile_definitions(cppqtgraph_widgets_groupbox_p5_22
        PRIVATE
            CPPQTGRAPH_P5_22_VISUAL_DIFF_DIR="${PROJECT_SOURCE_DIR}/reports/visual-diffs/GroupBox"
            CPPQTGRAPH_P5_22_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P5.22"
    )
    set_tests_properties(cppqtgraph.widgets.GroupBox.P5_22 PROPERTIES LABELS "visual;P5.22")
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_filedialog_pathbutton_p5_23
        widgets/test_FileDialog_PathButton_P5_23.cpp
        cppqtgraph.widgets.FileDialog_PathButton.P5_23
    )
    set_target_properties(cppqtgraph_widgets_filedialog_pathbutton_p5_23 PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph_widgets_filedialog_pathbutton_p5_23 PRIVATE Qt6::Test)
    target_compile_definitions(cppqtgraph_widgets_filedialog_pathbutton_p5_23
        PRIVATE
            CPPQTGRAPH_P5_23_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P5.23"
    )
    set_tests_properties(cppqtgraph.widgets.FileDialog_PathButton.P5_23 PROPERTIES LABELS P5.23)
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_progressdialog_p5_24
        widgets/test_ProgressDialog_P5_24.cpp
        cppqtgraph.widgets.ProgressDialog.P5_24
    )
    set_target_properties(cppqtgraph_widgets_progressdialog_p5_24 PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph_widgets_progressdialog_p5_24 PRIVATE Qt6::Test)
    target_compile_definitions(cppqtgraph_widgets_progressdialog_p5_24
        PRIVATE
            CPPQTGRAPH_P5_24_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P5.24"
    )
    set_tests_properties(cppqtgraph.widgets.ProgressDialog.P5_24 PROPERTIES LABELS P5.24)
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_feedbackbutton_busycursor_p5_25
        widgets/test_FeedbackButton_BusyCursor_P5_25.cpp
        cppqtgraph.widgets.FeedbackButton_BusyCursor.P5_25
    )
    set_target_properties(cppqtgraph_widgets_feedbackbutton_busycursor_p5_25 PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph_widgets_feedbackbutton_busycursor_p5_25 PRIVATE Qt6::Test)
    target_compile_definitions(cppqtgraph_widgets_feedbackbutton_busycursor_p5_25
        PRIVATE
            CPPQTGRAPH_P5_25_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P5.25"
    )
    set_tests_properties(cppqtgraph.widgets.FeedbackButton_BusyCursor.P5_25 PROPERTIES LABELS P5.25)
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_imageview_rawimagewidget_p5_07
        widgets/test_ImageView_RawImageWidget_P5_07.cpp
        cppqtgraph.widgets.ImageView_RawImageWidget.P5_07
    )
    set_target_properties(cppqtgraph_widgets_imageview_rawimagewidget_p5_07 PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph_widgets_imageview_rawimagewidget_p5_07 PRIVATE Qt6::Widgets)
    target_compile_definitions(cppqtgraph_widgets_imageview_rawimagewidget_p5_07
        PRIVATE
            CPPQTGRAPH_P5_07_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P5.07"
            CPPQTGRAPH_P5_07_REPOSITORY_REPORT_DIR="${PROJECT_SOURCE_DIR}/reports/issues/P5.07"
    )
    set_tests_properties(cppqtgraph.widgets.ImageView_RawImageWidget.P5_07 PROPERTIES LABELS P5.07)
    cppqtgraph_add_cpp_test(
        cppqtgraph_widgets_scatterplotwidget_p5_08
        widgets/test_ScatterPlotWidget_P5_08.cpp
        cppqtgraph.widgets.ScatterPlotWidget.P5_08
    )
    set_target_properties(cppqtgraph_widgets_scatterplotwidget_p5_08 PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph_widgets_scatterplotwidget_p5_08 PRIVATE Qt6::Test)
    target_compile_definitions(cppqtgraph_widgets_scatterplotwidget_p5_08
        PRIVATE
            CPPQTGRAPH_P5_08_REPOSITORY_REPORT_DIR="${PROJECT_BINARY_DIR}/reports/issues/P5.08"
    )
    set_tests_properties(cppqtgraph.widgets.ScatterPlotWidget.P5_08 PROPERTIES LABELS P5.08)
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_plotcurveitem
        graphicsItems/test_PlotCurveItem.cpp
        cppqtgraph.graphicsItems.PlotCurveItem
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_plotcurveitem_setdata
        graphicsItems/test_PlotCurveItem_setData.cpp
        cppqtgraph.graphicsItems.PlotCurveItem.setData
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_plotdataitem
        graphicsItems/test_PlotDataItem.cpp
        cppqtgraph.graphicsItems.PlotDataItem
    )
    set_tests_properties(cppqtgraph.graphicsItems.PlotDataItem PROPERTIES LABELS P3.10)
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_scatterplotitem
        graphicsItems/test_ScatterPlotItem.cpp
        cppqtgraph.graphicsItems.ScatterPlotItem
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_plotdataitem_logmode
        graphicsItems/test_PlotDataItem_logMode.cpp
        cppqtgraph.graphicsItems.PlotDataItem.logMode
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_plotitem_logmode
        graphicsItems/test_PlotItem_logMode.cpp
        cppqtgraph.graphicsItems.PlotItem.logMode
    )

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_graphitem_curvepoint_p4_03
        graphicsItems/test_GraphItem_CurvePoint_P4_03.cpp
        cppqtgraph.graphicsItems.GraphItem_CurvePoint.P4_03
    )
    set_tests_properties(cppqtgraph.graphicsItems.GraphItem_CurvePoint.P4_03 PROPERTIES LABELS P4.03)

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_boxplot_pcolormesh_p4_04
        graphicsItems/test_Boxplot_PColorMesh_P4_04.cpp
        cppqtgraph.graphicsItems.Boxplot_PColorMesh.P4_04
    )
    target_compile_definitions(cppqtgraph_graphicsitems_boxplot_pcolormesh_p4_04
        PRIVATE
            CPPQTGRAPH_P4_04_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P4_04/boxplot_pcolormesh_oracle.json"
            CPPQTGRAPH_P4_04_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P4.04"
    )
    set_tests_properties(cppqtgraph.graphicsItems.Boxplot_PColorMesh.P4_04 PROPERTIES LABELS P4.04)

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_imageitem_render
        graphicsItems/test_ImageItem_render.cpp
        cppqtgraph.graphicsItems.ImageItem.render
    )
    target_compile_definitions(cppqtgraph_graphicsitems_imageitem_render
        PRIVATE
            CPPQTGRAPH_P2_13_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual-diffs/ImageItem/P2.13"
    )
    set_tests_properties(cppqtgraph.graphicsItems.ImageItem.render PROPERTIES LABELS "visual;P2.13")

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_imageitem_nonuniform_p4_05
        graphicsItems/test_ImageItem_NonUniform_P4_05.cpp
        cppqtgraph.graphicsItems.ImageItem_NonUniform.P4_05
    )
    target_compile_definitions(cppqtgraph_graphicsitems_imageitem_nonuniform_p4_05
        PRIVATE
            CPPQTGRAPH_P4_05_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P4.05"
    )
    set_tests_properties(cppqtgraph.graphicsItems.ImageItem_NonUniform.P4_05 PROPERTIES LABELS P4.05)

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_histogramlut_colorbar_p4_06
        graphicsItems/test_HistogramLUT_ColorBar_P4_06.cpp
        cppqtgraph.graphicsItems.HistogramLUT_ColorBar.P4_06
    )
    target_compile_definitions(cppqtgraph_graphicsitems_histogramlut_colorbar_p4_06
        PRIVATE
            CPPQTGRAPH_P4_06_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P4.06"
    )
    set_tests_properties(cppqtgraph.graphicsItems.HistogramLUT_ColorBar.P4_06 PROPERTIES LABELS P4.06)

    cppqtgraph_add_cpp_test(
        cppqtgraph_graphicsitems_isocurveitem_p4_07
        graphicsItems/test_IsocurveItem_P4_07.cpp
        cppqtgraph.graphicsItems.IsocurveItem.P4_07
    )
    target_compile_definitions(cppqtgraph_graphicsitems_isocurveitem_p4_07
        PRIVATE
            CPPQTGRAPH_P4_07_FIXTURE="${PROJECT_SOURCE_DIR}/oracle/fixtures/P4_07/isocurve_oracle.json"
            CPPQTGRAPH_P4_07_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/artifacts/P4.07"
    )
    set_tests_properties(cppqtgraph.graphicsItems.IsocurveItem.P4_07 PROPERTIES LABELS P4.07)

    add_executable(cppqtgraph_examples_simpleplot "${PROJECT_SOURCE_DIR}/examples/SimplePlot.cpp")
    cppqtgraph_configure_cpp_test_target(cppqtgraph_examples_simpleplot)
    add_executable(cppqtgraph_examples_imageitem "${PROJECT_SOURCE_DIR}/examples/ImageItem.cpp")
    cppqtgraph_configure_cpp_test_target(cppqtgraph_examples_imageitem)
    add_executable(cppqtgraph_examples_cliexample "${PROJECT_SOURCE_DIR}/examples/CLIexample.cpp")
    cppqtgraph_configure_cpp_test_target(cppqtgraph_examples_cliexample)
    add_executable(cppqtgraph_examples_plotting "${PROJECT_SOURCE_DIR}/examples/Plotting.cpp")
    cppqtgraph_configure_cpp_test_target(cppqtgraph_examples_plotting)
    add_executable(cppqtgraph_examples_imageview "${PROJECT_SOURCE_DIR}/examples/ImageView.cpp")
    cppqtgraph_configure_cpp_test_target(cppqtgraph_examples_imageview)
    add_executable(cppqtgraph_examples_exampleapp
        "${PROJECT_SOURCE_DIR}/examples/ExampleRegistry.cpp"
        "${PROJECT_SOURCE_DIR}/examples/ExampleApp.cpp"
    )
    cppqtgraph_configure_cpp_test_target(cppqtgraph_examples_exampleapp)

    cppqtgraph_add_cpp_test(
        cppqtgraph_visual_plotcurveitem_bounds
        visual/P1_08_plotcurveitem_bounds.cpp
        P1.08.visual.PlotCurveItem.bounds
    )
    set_tests_properties(P1.08.visual.PlotCurveItem.bounds PROPERTIES LABELS "visual;P1.08")

    cppqtgraph_add_cpp_test(
        cppqtgraph_visual_plotcurveitem_p3_09
        visual/P3_09_plotcurveitem_paint.cpp
        P3.09.visual.PlotCurveItem.paint
    )
    target_compile_definitions(cppqtgraph_visual_plotcurveitem_p3_09
        PRIVATE
            CPPQTGRAPH_P3_09_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual/P3.09"
            CPPQTGRAPH_P3_09_CANONICAL_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual-diffs/P3.09-PlotCurveItem"
    )
    set_tests_properties(P3.09.visual.PlotCurveItem.paint PROPERTIES LABELS "visual;P3.09")

    cppqtgraph_add_cpp_test(
        cppqtgraph_visual_scatterplotitem_p4_01
        visual/P4_01_scatterplotitem_symbols.cpp
        P4.01.visual.ScatterPlotItem.symbol_atlas
    )
    target_compile_definitions(cppqtgraph_visual_scatterplotitem_p4_01
        PRIVATE
            CPPQTGRAPH_P4_01_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual/P4.01"
            CPPQTGRAPH_P4_01_CANONICAL_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual-diffs/P4.01-ScatterPlotItem"
    )
    set_tests_properties(P4.01.visual.ScatterPlotItem.symbol_atlas PROPERTIES LABELS "visual;P4.01")

    cppqtgraph_add_cpp_test(
        cppqtgraph_visual_bar_error_fillbetween_p4_02
        visual/P4_02_bar_error_fillbetween.cpp
        P4.02.visual.BarGraph_ErrorBar_FillBetween
    )
    target_compile_definitions(cppqtgraph_visual_bar_error_fillbetween_p4_02
        PRIVATE
            CPPQTGRAPH_P4_02_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual/P4.02"
            CPPQTGRAPH_P4_02_CANONICAL_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual-diffs/P4.02-bar-error-fillbetween"
            CPPQTGRAPH_P4_02_REPOSITORY_ARTIFACT_DIR="${PROJECT_SOURCE_DIR}/reports/visual/P4.02"
            CPPQTGRAPH_P4_02_REPOSITORY_CANONICAL_ARTIFACT_DIR="${PROJECT_SOURCE_DIR}/reports/visual-diffs/P4.02-bar-error-fillbetween"
    )
    set_tests_properties(P4.02.visual.BarGraph_ErrorBar_FillBetween PROPERTIES LABELS "visual;P4.02")

    cppqtgraph_add_cpp_test(
        cppqtgraph_visual_legend_grid_vtick_p4_11
        visual/P4_11_legend_grid_vtick.cpp
        P4.11.visual.Legend_Grid_VTickGroup
    )
    target_compile_definitions(cppqtgraph_visual_legend_grid_vtick_p4_11
        PRIVATE
            CPPQTGRAPH_P4_11_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual/P4.11"
            CPPQTGRAPH_P4_11_CANONICAL_ARTIFACT_DIR="${PROJECT_BINARY_DIR}/reports/visual-diffs/P4.11-legend-grid-vtick"
    )
    set_tests_properties(P4.11.visual.Legend_Grid_VTickGroup PROPERTIES LABELS "visual;P4.11")

    add_executable(cppqtgraph_visual_plotitem_p3_11 visual/P3_11_plotitem_layout_visual.cpp)
    cppqtgraph_configure_cpp_test_target(cppqtgraph_visual_plotitem_p3_11)
    add_test(
        NAME P3.11.visual.PlotItem.layout_axes_legend
        COMMAND "${Python3_EXECUTABLE}" -m pytest -q
            "${PROJECT_SOURCE_DIR}/tests/visual/test_P3_11_plotitem_layout_visual.py"
    )
    set_tests_properties(
        P3.11.visual.PlotItem.layout_axes_legend
        PROPERTIES
            LABELS "visual;P3.11"
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;PG_CPP_P3_11_VISUAL_RENDERER=$<TARGET_FILE:cppqtgraph_visual_plotitem_p3_11>;PG_P3_11_VISUAL_REPORTS_ROOT=${PROJECT_BINARY_DIR}/reports/visual/P3.11"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )

    add_executable(cppqtgraph_visual_render_example visual/P1_08_render_cpp_example.cpp)
    cppqtgraph_configure_cpp_test_target(cppqtgraph_visual_render_example)
    add_test(
        NAME P1.08.visual.SimplePlot
        COMMAND "${Python3_EXECUTABLE}" -m pytest -q
            "${PROJECT_SOURCE_DIR}/tests/visual/test_P1_08_cpp_visual_renderer.py"
    )
    set_tests_properties(
        P1.08.visual.SimplePlot
        PROPERTIES
            LABELS "visual;P1.08"
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;PG_CPP_VISUAL_RENDERER=$<TARGET_FILE:cppqtgraph_visual_render_example>;PG_VISUAL_REPORTS_ROOT=${PROJECT_BINARY_DIR}/reports/visual/P1.08;PG_VISUAL_REVIEW_REPORT=${PROJECT_SOURCE_DIR}/reports/visual/P1.08/SimplePlot/gpt5_vision_review.md"
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
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;PG_CPP_P3_13_VISUAL_RENDERER=$<TARGET_FILE:cppqtgraph_visual_render_example>;PG_P3_13_VISUAL_REPORTS_ROOT=${PROJECT_BINARY_DIR}/reports/visual-diffs;PG_P3_13_EXAMPLES_REPORTS_ROOT=${PROJECT_BINARY_DIR}/reports/examples/P3.13;PG_P3_13_VISUAL_REVIEW_REPORT=${PROJECT_SOURCE_DIR}/reports/examples/P3.13/gpt5_vision_review.md"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )

    cppqtgraph_add_cpp_test(
        cppqtgraph_examples_simpleplot_test
        examples/test_SimplePlot.cpp
        cppqtgraph.examples.SimplePlot
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_examples_imageitem_test
        examples/test_ImageItem.cpp
        cppqtgraph.examples.ImageItem
    )
    target_compile_definitions(cppqtgraph_examples_imageitem_test
        PRIVATE
            CPPQTGRAPH_P2_14_VISUAL_DIFF_DIR="${PROJECT_BINARY_DIR}/reports/visual-diffs/ImageItem-example"
    )
    set_tests_properties(cppqtgraph.examples.ImageItem PROPERTIES LABELS "visual;P2.14")
    cppqtgraph_add_cpp_test(
        cppqtgraph_examples_cliexample_test
        examples/test_CLIexample.cpp
        cppqtgraph.examples.CLIexample
    )
    set_tests_properties(cppqtgraph.examples.CLIexample PROPERTIES LABELS "visual")
    cppqtgraph_add_cpp_test(
        cppqtgraph_examples_plotting_test
        examples/test_Plotting.cpp
        cppqtgraph.examples.Plotting
    )
    set_tests_properties(cppqtgraph.examples.Plotting PROPERTIES LABELS "visual;interaction")
    cppqtgraph_add_cpp_test(
        cppqtgraph_examples_imageview_test
        examples/test_ImageView.cpp
        cppqtgraph.examples.ImageView
    )
    cppqtgraph_add_cpp_test(
        cppqtgraph_examples_exampleapp_test
        examples/test_ExampleApp.cpp
        cppqtgraph.examples.ExampleApp
    )
    target_link_libraries(cppqtgraph_examples_exampleapp_test PRIVATE Qt6::Test)
    add_dependencies(cppqtgraph_examples_exampleapp
        cppqtgraph_examples_simpleplot
        cppqtgraph_examples_imageitem
        cppqtgraph_examples_cliexample
        cppqtgraph_examples_plotting
        cppqtgraph_examples_imageview
    )
    add_dependencies(cppqtgraph_examples_exampleapp_test
        cppqtgraph_examples_simpleplot
        cppqtgraph_examples_imageitem
        cppqtgraph_examples_cliexample
        cppqtgraph_examples_plotting
        cppqtgraph_examples_imageview
    )
    set_tests_properties(cppqtgraph.examples.ExampleApp PROPERTIES LABELS "interaction")
    add_test(
        NAME P3.72.visual.Plotting
        COMMAND "${Python3_EXECUTABLE}" -m pytest -q
            "${PROJECT_SOURCE_DIR}/tests/examples/test_Plotting_P372_visual.py"
    )
    set_tests_properties(
        P3.72.visual.Plotting
        PROPERTIES
            LABELS "visual;P3.72"
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;PG_CPP_P372_VISUAL_RENDERER=$<TARGET_FILE:cppqtgraph_visual_render_example>;PG_P372_VISUAL_REPORTS_ROOT=${PROJECT_BINARY_DIR}/reports/visual-diffs;PG_P372_EXAMPLES_REPORTS_ROOT=${PROJECT_BINARY_DIR}/reports/examples/P3.72;PG_P372_VISUAL_REVIEW_REPORT=${PROJECT_SOURCE_DIR}/reports/examples/P3.72/gpt5_vision_review.md"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
    cppqtgraph_add_cpp_test(cppqtgraph_hierarchy_cpp hierarchy/test_cpp_hierarchy.cpp cppqtgraph.hierarchy.cpp)
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
