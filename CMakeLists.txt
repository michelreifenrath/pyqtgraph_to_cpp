cmake_minimum_required(VERSION 3.26)

project(CppQtGraph VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CppQtGraphOptions)
include(CppQtGraphWarnings)
include(CppQtGraphSanitizers)

cppqtgraph_configure_options()

add_library(cppqtgraph_project_options INTERFACE)
target_compile_features(cppqtgraph_project_options INTERFACE cxx_std_20)

cppqtgraph_create_warnings_target(cppqtgraph_project_warnings)

if(NOT TARGET Qt6::Core OR NOT TARGET Qt6::Gui OR NOT TARGET Qt6::Widgets)
    find_package(Qt6 6 COMPONENTS Core Gui Widgets QUIET)
endif()
set(_cppqtgraph_has_qtcore FALSE)
if(TARGET Qt6::Core)
    set(_cppqtgraph_has_qtcore TRUE)
elseif(CPPQTGRAPH_REQUIRE_QT)
    message(FATAL_ERROR "Qt 6 Core is required for PGCORE-003 Point. Install Qt 6 Core, or configure with -DCPPQTGRAPH_REQUIRE_QT=OFF only if intentionally skipping Qt-dependent targets.")
else()
    message(STATUS "Qt6 Core was not found; skipping the Point library source and test target")
endif()

set(_cppqtgraph_has_vector FALSE)
if(TARGET Qt6::Core AND TARGET Qt6::Gui)
    set(_cppqtgraph_has_vector TRUE)
elseif(CPPQTGRAPH_REQUIRE_QT)
    message(FATAL_ERROR "Qt 6 Gui is required for PGCORE-004 Vector. Install Qt 6 Gui, or configure with -DCPPQTGRAPH_REQUIRE_QT=OFF only if intentionally skipping Qt-dependent targets.")
else()
    message(STATUS "Qt6 Core/Gui was not found; skipping the Vector library source and test target")
endif()

set(_cppqtgraph_has_mkcolor FALSE)
if(TARGET Qt6::Core AND TARGET Qt6::Gui)
    set(_cppqtgraph_has_mkcolor TRUE)
elseif(CPPQTGRAPH_REQUIRE_QT)
    message(FATAL_ERROR "Qt 6 Gui is required for PGCORE-005 mkColor. Install Qt 6 Gui, or configure with -DCPPQTGRAPH_REQUIRE_QT=OFF only if intentionally skipping Qt-dependent targets.")
else()
    message(STATUS "Qt6 Core/Gui was not found; skipping the mkColor library source and test target")
endif()

set(_cppqtgraph_has_graphicsitem FALSE)
if(TARGET Qt6::Core AND TARGET Qt6::Gui AND TARGET Qt6::Widgets)
    set(_cppqtgraph_has_graphicsitem TRUE)
elseif(CPPQTGRAPH_REQUIRE_QT)
    message(FATAL_ERROR "Qt 6 Widgets is required for PGGI-001 GraphicsItem and PGGI-002 GraphicsObject. Install Qt 6 Widgets, or configure with -DCPPQTGRAPH_REQUIRE_QT=OFF only if intentionally skipping Qt Widgets-dependent targets.")
else()
    message(STATUS "Qt6 Widgets was not found; skipping the GraphicsItem/GraphicsObject library sources and test targets")
endif()

add_library(cppqtgraph
    src/cppqtgraph/core/ArrayView.cpp
    src/cppqtgraph/functions.cpp
    src/cppqtgraph/PlotData.cpp
)
set_target_properties(cppqtgraph PROPERTIES
    EXPORT_NAME CppQtGraph
    OUTPUT_NAME cppqtgraph
)
target_include_directories(cppqtgraph
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(cppqtgraph
    PUBLIC
        cppqtgraph_project_options
)
target_compile_options(cppqtgraph
    PRIVATE
        $<TARGET_PROPERTY:cppqtgraph_project_warnings,INTERFACE_COMPILE_OPTIONS>
)
if(_cppqtgraph_has_qtcore)
    target_sources(cppqtgraph
        PRIVATE
            include/cppqtgraph/SignalProxy.hpp
            include/cppqtgraph/ThreadsafeTimer.hpp
            src/cppqtgraph/Point.cpp
            src/cppqtgraph/SignalProxy.cpp
            src/cppqtgraph/ThreadsafeTimer.cpp
    )
    set_target_properties(cppqtgraph PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph
        PUBLIC
            Qt6::Core
    )
endif()

if(_cppqtgraph_has_vector)
    target_sources(cppqtgraph
        PRIVATE
            src/cppqtgraph/Vector.cpp
    )
    target_link_libraries(cppqtgraph
        PUBLIC
            Qt6::Gui
    )
endif()

if(_cppqtgraph_has_mkcolor)
    target_sources(cppqtgraph
        PRIVATE
            src/cppqtgraph/colormap.cpp
            src/cppqtgraph/colors/palette.cpp
            src/cppqtgraph/functions_qimage.cpp
    )
    target_compile_definitions(cppqtgraph
        PUBLIC
            CPPQTGRAPH_ENABLE_QT_COLOR=1
    )
    target_link_libraries(cppqtgraph
        PUBLIC
            Qt6::Gui
    )
endif()

if(_cppqtgraph_has_graphicsitem)
    target_sources(cppqtgraph
        PRIVATE
            include/cppqtgraph/WidgetGroup.hpp
            include/cppqtgraph/GraphicsScene/GraphicsScene.hpp
            include/cppqtgraph/GraphicsScene/mouseEvents.hpp
            src/cppqtgraph/WidgetGroup.cpp
            src/cppqtgraph/GraphicsScene/GraphicsScene.cpp
            src/cppqtgraph/GraphicsScene/mouseEvents.cpp
            src/cppqtgraph/graphicsItems/GraphicsItem.cpp
            src/cppqtgraph/graphicsItems/GraphicsObject.cpp
            src/cppqtgraph/graphicsItems/GraphicsWidget.cpp
            src/cppqtgraph/graphicsItems/GraphicsWidgetAnchor.cpp
            include/cppqtgraph/graphicsItems/ButtonItem.hpp
            src/cppqtgraph/graphicsItems/ButtonItem.cpp
            include/cppqtgraph/icons/graphIcons.hpp
            src/cppqtgraph/icons/graphIcons.cpp
            include/cppqtgraph/graphicsItems/GraphicsLayout.hpp
            src/cppqtgraph/graphicsItems/AxisItem.cpp
            include/cppqtgraph/graphicsItems/DateAxisItem.hpp
            src/cppqtgraph/graphicsItems/DateAxisItem.cpp
            include/cppqtgraph/graphicsItems/GridItem.hpp
            src/cppqtgraph/graphicsItems/GridItem.cpp
            include/cppqtgraph/graphicsItems/LegendItem.hpp
            src/cppqtgraph/graphicsItems/LegendItem.cpp
            include/cppqtgraph/graphicsItems/TextItem.hpp
            src/cppqtgraph/graphicsItems/TextItem.cpp
            include/cppqtgraph/graphicsItems/ScaleBar.hpp
            src/cppqtgraph/graphicsItems/ScaleBar.cpp
            include/cppqtgraph/graphicsItems/LabelItem.hpp
            src/cppqtgraph/graphicsItems/LabelItem.cpp
            include/cppqtgraph/graphicsItems/VTickGroup.hpp
            src/cppqtgraph/graphicsItems/VTickGroup.cpp
            include/cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp
            src/cppqtgraph/graphicsItems/ViewBox/ViewBox.cpp
            include/cppqtgraph/graphicsItems/ViewBox/axisCtrlTemplate_generic.hpp
            src/cppqtgraph/graphicsItems/ViewBox/axisCtrlTemplate_generic.cpp
            include/cppqtgraph/graphicsItems/ViewBox/ViewBoxMenu.hpp
            src/cppqtgraph/graphicsItems/ViewBox/ViewBoxMenu.cpp
            include/cppqtgraph/graphicsItems/PlotItem/plotConfigTemplate_generic.hpp
            src/cppqtgraph/graphicsItems/PlotItem/plotConfigTemplate_generic.cpp
            src/cppqtgraph/graphicsItems/PlotItem/PlotItem.cpp
            include/cppqtgraph/widgets/GraphicsView.hpp
            src/cppqtgraph/widgets/GraphicsView.cpp
            include/cppqtgraph/widgets/GraphicsLayoutWidget.hpp
            include/cppqtgraph/widgets/PlotWidget.hpp
            include/cppqtgraph/widgets/HistogramLUTWidget.hpp
            src/cppqtgraph/widgets/HistogramLUTWidget.cpp
            include/cppqtgraph/widgets/ColorButton.hpp
            src/cppqtgraph/widgets/ColorButton.cpp
            include/cppqtgraph/widgets/ColorMapButton.hpp
            src/cppqtgraph/widgets/ColorMapButton.cpp
            include/cppqtgraph/widgets/ColorMapMenu.hpp
            src/cppqtgraph/widgets/ColorMapMenu.cpp
            include/cppqtgraph/widgets/ColorMapWidget.hpp
            src/cppqtgraph/widgets/ColorMapWidget.cpp
            include/cppqtgraph/widgets/GradientWidget.hpp
            src/cppqtgraph/widgets/GradientWidget.cpp
            include/cppqtgraph/widgets/TreeWidget.hpp
            src/cppqtgraph/widgets/TreeWidget.cpp
            include/cppqtgraph/widgets/CheckTable.hpp
            src/cppqtgraph/widgets/CheckTable.cpp
            include/cppqtgraph/widgets/DataTreeWidget.hpp
            src/cppqtgraph/widgets/DataTreeWidget.cpp
            include/cppqtgraph/widgets/DiffTreeWidget.hpp
            src/cppqtgraph/widgets/DiffTreeWidget.cpp
            include/cppqtgraph/widgets/TableWidget.hpp
            src/cppqtgraph/widgets/TableWidget.cpp
            include/cppqtgraph/widgets/ComboBox.hpp
            src/cppqtgraph/widgets/ComboBox.cpp
            include/cppqtgraph/widgets/SpinBox.hpp
            src/cppqtgraph/widgets/SpinBox.cpp
            include/cppqtgraph/widgets/JoystickButton.hpp
            src/cppqtgraph/widgets/JoystickButton.cpp
            include/cppqtgraph/widgets/VerticalLabel.hpp
            src/cppqtgraph/widgets/VerticalLabel.cpp
            include/cppqtgraph/widgets/LayoutWidget.hpp
            src/cppqtgraph/widgets/LayoutWidget.cpp
            include/cppqtgraph/widgets/GroupBox.hpp
            src/cppqtgraph/widgets/GroupBox.cpp
            include/cppqtgraph/widgets/FileDialog.hpp
            src/cppqtgraph/widgets/FileDialog.cpp
            include/cppqtgraph/widgets/PathButton.hpp
            src/cppqtgraph/widgets/PathButton.cpp
            include/cppqtgraph/widgets/ProgressDialog.hpp
            src/cppqtgraph/widgets/ProgressDialog.cpp
            include/cppqtgraph/widgets/FeedbackButton.hpp
            src/cppqtgraph/widgets/FeedbackButton.cpp
            include/cppqtgraph/widgets/BusyCursor.hpp
            src/cppqtgraph/widgets/BusyCursor.cpp
            include/cppqtgraph/widgets/RawImageWidget.hpp
            src/cppqtgraph/widgets/RawImageWidget.cpp
            include/cppqtgraph/widgets/ScatterPlotWidget.hpp
            src/cppqtgraph/widgets/ScatterPlotWidget.cpp
            include/cppqtgraph/imageview/ImageView.hpp
            src/cppqtgraph/imageview/ImageView.cpp
            include/cppqtgraph/imageview/ImageViewTemplate_generic.hpp
            src/cppqtgraph/imageview/ImageViewTemplate_generic.cpp
            src/cppqtgraph/widgets/PlotWidget.cpp
            include/cppqtgraph/graphicsItems/PlotDataItem.hpp
            src/cppqtgraph/graphicsItems/PlotDataItem.cpp
            include/cppqtgraph/graphicsItems/ScatterPlotItem.hpp
            src/cppqtgraph/graphicsItems/ScatterPlotItem.cpp
            include/cppqtgraph/graphicsItems/GraphItem.hpp
            src/cppqtgraph/graphicsItems/GraphItem.cpp
            include/cppqtgraph/graphicsItems/BoxplotItem.hpp
            src/cppqtgraph/graphicsItems/BoxplotItem.cpp
            include/cppqtgraph/graphicsItems/PColorMeshItem.hpp
            src/cppqtgraph/graphicsItems/PColorMeshItem.cpp
            include/cppqtgraph/graphicsItems/CurvePoint.hpp
            src/cppqtgraph/graphicsItems/CurvePoint.cpp
            include/cppqtgraph/graphicsItems/ROI.hpp
            src/cppqtgraph/graphicsItems/ROI.cpp
            include/cppqtgraph/graphicsItems/InfiniteLine.hpp
            src/cppqtgraph/graphicsItems/InfiniteLine.cpp
            include/cppqtgraph/graphicsItems/LinearRegionItem.hpp
            include/cppqtgraph/graphicsItems/TargetItem.hpp
            include/cppqtgraph/graphicsItems/BarGraphItem.hpp
            include/cppqtgraph/graphicsItems/ErrorBarItem.hpp
            include/cppqtgraph/graphicsItems/FillBetweenItem.hpp
            include/cppqtgraph/graphicsItems/ImageItem.hpp
            src/cppqtgraph/graphicsItems/ImageItem.cpp
            include/cppqtgraph/graphicsItems/HistogramLUTItem.hpp
            src/cppqtgraph/graphicsItems/HistogramLUTItem.cpp
            include/cppqtgraph/graphicsItems/ColorBarItem.hpp
            src/cppqtgraph/graphicsItems/ColorBarItem.cpp
            include/cppqtgraph/graphicsItems/GradientEditorItem.hpp
            src/cppqtgraph/graphicsItems/GradientEditorItem.cpp
            include/cppqtgraph/graphicsItems/GradientLegend.hpp
            src/cppqtgraph/graphicsItems/GradientLegend.cpp
            include/cppqtgraph/graphicsItems/NonUniformImage.hpp
            src/cppqtgraph/graphicsItems/NonUniformImage.cpp
            include/cppqtgraph/graphicsItems/IsocurveItem.hpp
            src/cppqtgraph/graphicsItems/IsocurveItem.cpp
            src/cppqtgraph/graphicsItems/PlotCurveItem.cpp
    )
    set_target_properties(cppqtgraph PROPERTIES AUTOMOC ON)
    target_link_libraries(cppqtgraph
        PUBLIC
            Qt6::Widgets
    )
endif()

set(_cppqtgraph_public_qt_components)
if(_cppqtgraph_has_qtcore)
    list(APPEND _cppqtgraph_public_qt_components Core)
endif()
if(_cppqtgraph_has_vector OR _cppqtgraph_has_mkcolor)
    list(APPEND _cppqtgraph_public_qt_components Gui)
endif()
if(_cppqtgraph_has_graphicsitem)
    list(APPEND _cppqtgraph_public_qt_components Widgets)
endif()
list(REMOVE_DUPLICATES _cppqtgraph_public_qt_components)
set(CPPQTGRAPH_PACKAGE_QT_COMPONENTS "${_cppqtgraph_public_qt_components}")
set(CPPQTGRAPH_PACKAGE_REQUIRE_OPENCV "${CPPQTGRAPH_HAS_OPENCV}")

set(_cppqtgraph_package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/CppQtGraph")
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CppQtGraphConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CppQtGraphConfig.cmake"
    INSTALL_DESTINATION "${_cppqtgraph_package_dir}"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/CppQtGraphConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

install(TARGETS
    cppqtgraph
    cppqtgraph_project_options
    EXPORT CppQtGraphTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.hpp"
)
install(EXPORT CppQtGraphTargets
    FILE CppQtGraphTargets.cmake
    NAMESPACE CppQtGraph::
    DESTINATION "${_cppqtgraph_package_dir}"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/CppQtGraphConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/CppQtGraphConfigVersion.cmake"
    DESTINATION "${_cppqtgraph_package_dir}"
)

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_subdirectory(tests)
endif()
