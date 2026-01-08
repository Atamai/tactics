cmake_minimum_required(VERSION 3.16)
project(cbGUI LANGUAGES CXX)

# ------------------------------------------------------------------------------
# CMake Policies
# ------------------------------------------------------------------------------
if(POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW)
endif()

# ------------------------------------------------------------------------------
# Qt Setup
# ------------------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Svg Xml)

# ------------------------------------------------------------------------------
# VTK Setup
# ------------------------------------------------------------------------------
find_package(VTK 9.6 REQUIRED
    COMPONENTS
        CommonCore
        CommonDataModel
        FiltersCore
        FiltersGeneral
        IOImage
)
find_package(ToolCursor REQUIRED)

# ------------------------------------------------------------------------------
# Source files
# ------------------------------------------------------------------------------
set(LIB_SRCS
    qvtkViewToolCursorWidget.cxx
    cbMainWindow.cxx
    cbApplicationController.cxx
    cbQtDicomDirDialog.cxx
    cbQtDicomDirModel.cxx
    cbQtDicomDirThread.cxx
    cbQtDicomDirView.cxx
    cbQtVTKOutputWindow.cxx
    cbStage.cxx
    cbStageManager.cxx
)

set(LIB_HDRS
    qvtkViewToolCursorWidget.h
    cbMainWindow.h
    cbApplicationController.h
    cbStage.h
    cbStageManager.h
)

# ------------------------------------------------------------------------------
# Create library
# ------------------------------------------------------------------------------
add_library(${PROJECT_NAME} STATIC ${LIB_SRCS} ${LIB_HDRS})

# ------------------------------------------------------------------------------
# Include directories (single root for all project headers)
# ------------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"                # root: gui/, data/, view/
)

# ------------------------------------------------------------------------------
# Link Qt libraries
# ------------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Svg
    Qt6::Xml
)

# ------------------------------------------------------------------------------
# Link VTK libraries (modern targets)
# ------------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    VTK::CommonCore
    VTK::CommonDataModel
    VTK::FiltersCore
    VTK::FiltersGeneral
    VTK::IOImage
)

# ------------------------------------------------------------------------------
# Link other internal libraries
# ------------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    cbView
    cbData
    cbVTK
)

# ------------------------------------------------------------------------------
# VTK automatic module initialization
# ------------------------------------------------------------------------------
vtk_module_autoinit(
    TARGETS ${PROJECT_NAME}
    MODULES
        VTK::CommonCore
        VTK::CommonDataModel
        VTK::FiltersCore
        VTK::FiltersGeneral
        VTK::IOImage
)

# ------------------------------------------------------------------------------
# Export to parent scope
# ------------------------------------------------------------------------------
set(Cerebra_LIBRARIES
    ${Cerebra_LIBRARIES} ${PROJECT_NAME}
    PARENT_SCOPE
)

set(INCLUDE_OUTPUT_PATH
    ${INCLUDE_OUTPUT_PATH} "${CMAKE_CURRENT_SOURCE_DIR}"
    PARENT_SCOPE
)

export(
  TARGETS ${PROJECT_NAME}
  APPEND
  FILE "${CONFIGURE_OUTPUT_PATH}/CerebraTargets.cmake"
)
