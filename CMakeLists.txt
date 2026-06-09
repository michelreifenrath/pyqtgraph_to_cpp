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
            include/pyqtgraph/SignalProxy.hpp
            include/pyqtgraph/ThreadsafeTimer.hpp
            src/pyqtgraph/Point.cpp
            src/pyqtgraph/SignalProxy.cpp
            src/pyqtgraph/ThreadsafeTimer.cpp
    )
    set_target_properties(pyqtgraph_cpp PROPERTIES AUTOMOC ON)
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
            src/pyqtgraph/colors/palette.cpp
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
            include/pyqtgraph/WidgetGroup.hpp
            include/pyqtgraph/GraphicsScene/GraphicsScene.hpp
            include/pyqtgraph/GraphicsScene/mouseEvents.hpp
            src/pyqtgraph/WidgetGroup.cpp
            src/pyqtgraph/GraphicsScene/GraphicsScene.cpp
            src/pyqtgraph/GraphicsScene/mouseEvents.cpp
            src/pyqtgraph/graphicsItems/GraphicsItem.cpp
            src/pyqtgraph/graphicsItems/GraphicsObject.cpp
            src/pyqtgraph/graphicsItems/GraphicsWidget.cpp
            src/pyqtgraph/graphicsItems/GraphicsWidgetAnchor.cpp
            include/pyqtgraph/graphicsItems/ButtonItem.hpp
            src/pyqtgraph/graphicsItems/ButtonItem.cpp
            include/pyqtgraph/graphicsItems/GraphicsLayout.hpp
            src/pyqtgraph/graphicsItems/AxisItem.cpp
            include/pyqtgraph/graphicsItems/DateAxisItem.hpp
            src/pyqtgraph/graphicsItems/DateAxisItem.cpp
            include/pyqtgraph/graphicsItems/GridItem.hpp
            src/pyqtgraph/graphicsItems/GridItem.cpp
            include/pyqtgraph/graphicsItems/LegendItem.hpp
            src/pyqtgraph/graphicsItems/LegendItem.cpp
            include/pyqtgraph/graphicsItems/TextItem.hpp
            src/pyqtgraph/graphicsItems/TextItem.cpp
            include/pyqtgraph/graphicsItems/ScaleBar.hpp
            src/pyqtgraph/graphicsItems/ScaleBar.cpp
            include/pyqtgraph/graphicsItems/LabelItem.hpp
            src/pyqtgraph/graphicsItems/LabelItem.cpp
            include/pyqtgraph/graphicsItems/VTickGroup.hpp
            src/pyqtgraph/graphicsItems/VTickGroup.cpp
            include/pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp
            src/pyqtgraph/graphicsItems/ViewBox/ViewBox.cpp
            include/pyqtgraph/graphicsItems/PlotItem/plotConfigTemplate_generic.hpp
            src/pyqtgraph/graphicsItems/PlotItem/plotConfigTemplate_generic.cpp
            src/pyqtgraph/graphicsItems/PlotItem/PlotItem.cpp
            include/pyqtgraph/widgets/GraphicsView.hpp
            src/pyqtgraph/widgets/GraphicsView.cpp
            include/pyqtgraph/widgets/GraphicsLayoutWidget.hpp
            include/pyqtgraph/widgets/ColorButton.hpp
            src/pyqtgraph/widgets/ColorButton.cpp
            include/pyqtgraph/widgets/ColorMapButton.hpp
            src/pyqtgraph/widgets/ColorMapButton.cpp
            include/pyqtgraph/widgets/ColorMapMenu.hpp
            src/pyqtgraph/widgets/ColorMapMenu.cpp
            include/pyqtgraph/widgets/ColorMapWidget.hpp
            src/pyqtgraph/widgets/ColorMapWidget.cpp
            include/pyqtgraph/widgets/GradientWidget.hpp
            src/pyqtgraph/widgets/GradientWidget.cpp
            include/pyqtgraph/widgets/TreeWidget.hpp
            src/pyqtgraph/widgets/TreeWidget.cpp
            include/pyqtgraph/widgets/CheckTable.hpp
            src/pyqtgraph/widgets/CheckTable.cpp
            include/pyqtgraph/widgets/DataTreeWidget.hpp
            src/pyqtgraph/widgets/DataTreeWidget.cpp
            include/pyqtgraph/widgets/DiffTreeWidget.hpp
            src/pyqtgraph/widgets/DiffTreeWidget.cpp
            include/pyqtgraph/widgets/TableWidget.hpp
            src/pyqtgraph/widgets/TableWidget.cpp
            include/pyqtgraph/widgets/ComboBox.hpp
            src/pyqtgraph/widgets/ComboBox.cpp
            include/pyqtgraph/widgets/SpinBox.hpp
            src/pyqtgraph/widgets/SpinBox.cpp
            include/pyqtgraph/widgets/JoystickButton.hpp
            src/pyqtgraph/widgets/JoystickButton.cpp
            include/pyqtgraph/widgets/VerticalLabel.hpp
            src/pyqtgraph/widgets/VerticalLabel.cpp
            include/pyqtgraph/widgets/LayoutWidget.hpp
            src/pyqtgraph/widgets/LayoutWidget.cpp
            include/pyqtgraph/widgets/GroupBox.hpp
            src/pyqtgraph/widgets/GroupBox.cpp
            include/pyqtgraph/widgets/FileDialog.hpp
            src/pyqtgraph/widgets/FileDialog.cpp
            include/pyqtgraph/widgets/PathButton.hpp
            src/pyqtgraph/widgets/PathButton.cpp
            include/pyqtgraph/widgets/ProgressDialog.hpp
            src/pyqtgraph/widgets/ProgressDialog.cpp
            include/pyqtgraph/widgets/FeedbackButton.hpp
            src/pyqtgraph/widgets/FeedbackButton.cpp
            include/pyqtgraph/widgets/BusyCursor.hpp
            src/pyqtgraph/widgets/BusyCursor.cpp
            include/pyqtgraph/widgets/RawImageWidget.hpp
            src/pyqtgraph/widgets/RawImageWidget.cpp
            include/pyqtgraph/imageview/ImageView.hpp
            src/pyqtgraph/imageview/ImageView.cpp
            include/pyqtgraph/imageview/ImageViewTemplate_generic.hpp
            src/pyqtgraph/imageview/ImageViewTemplate_generic.cpp
            src/pyqtgraph/widgets/PlotWidget.cpp
            include/pyqtgraph/graphicsItems/PlotDataItem.hpp
            src/pyqtgraph/graphicsItems/PlotDataItem.cpp
            include/pyqtgraph/graphicsItems/ScatterPlotItem.hpp
            src/pyqtgraph/graphicsItems/ScatterPlotItem.cpp
            include/pyqtgraph/graphicsItems/GraphItem.hpp
            src/pyqtgraph/graphicsItems/GraphItem.cpp
            include/pyqtgraph/graphicsItems/BoxplotItem.hpp
            src/pyqtgraph/graphicsItems/BoxplotItem.cpp
            include/pyqtgraph/graphicsItems/PColorMeshItem.hpp
            src/pyqtgraph/graphicsItems/PColorMeshItem.cpp
            include/pyqtgraph/graphicsItems/CurvePoint.hpp
            src/pyqtgraph/graphicsItems/CurvePoint.cpp
            include/pyqtgraph/graphicsItems/ROI.hpp
            src/pyqtgraph/graphicsItems/ROI.cpp
            include/pyqtgraph/graphicsItems/InfiniteLine.hpp
            src/pyqtgraph/graphicsItems/InfiniteLine.cpp
            include/pyqtgraph/graphicsItems/LinearRegionItem.hpp
            include/pyqtgraph/graphicsItems/TargetItem.hpp
            include/pyqtgraph/graphicsItems/BarGraphItem.hpp
            include/pyqtgraph/graphicsItems/ErrorBarItem.hpp
            include/pyqtgraph/graphicsItems/FillBetweenItem.hpp
            include/pyqtgraph/graphicsItems/ImageItem.hpp
            src/pyqtgraph/graphicsItems/ImageItem.cpp
            include/pyqtgraph/graphicsItems/HistogramLUTItem.hpp
            src/pyqtgraph/graphicsItems/HistogramLUTItem.cpp
            include/pyqtgraph/graphicsItems/ColorBarItem.hpp
            src/pyqtgraph/graphicsItems/ColorBarItem.cpp
            include/pyqtgraph/graphicsItems/GradientEditorItem.hpp
            src/pyqtgraph/graphicsItems/GradientEditorItem.cpp
            include/pyqtgraph/graphicsItems/GradientLegend.hpp
            src/pyqtgraph/graphicsItems/GradientLegend.cpp
            include/pyqtgraph/graphicsItems/NonUniformImage.hpp
            src/pyqtgraph/graphicsItems/NonUniformImage.cpp
            include/pyqtgraph/graphicsItems/IsocurveItem.hpp
            src/pyqtgraph/graphicsItems/IsocurveItem.cpp
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
endif()
