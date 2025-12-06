cmake_minimum_required(VERSION 3.16)
project(cbGUI LANGUAGES CXX)

# ------------------------------------------------------------------------------
# Policies
# ------------------------------------------------------------------------------
if(POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW)
endif()

# ------------------------------------------------------------------------------
# Library name, dependencies, and sources
# ------------------------------------------------------------------------------
set(LIB_NAME ${PROJECT_NAME})

set(LIB_DEPS
  vtkCommon
  vtkRendering
  vtkToolCursor
  cbView
  cbData
  cbVTK
)

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

set(CEREBRA_QT_HEADER
  qvtkViewToolCursorWidget.h
  cbMainWindow.h
  cbApplicationController.h
  cbStage.h
  cbStageManager.h
)

# ------------------------------------------------------------------------------
# Create the library FIRST
# ------------------------------------------------------------------------------
add_library(${LIB_NAME} ${LIB_SRCS} ${CEREBRA_QT_HEADER})

# ------------------------------------------------------------------------------
# Include paths (must come AFTER add_library)
# ------------------------------------------------------------------------------
target_include_directories(${LIB_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${MAINFOLDER}/src/view"
  "${MAINFOLDER}/src/data"
  "${MAINFOLDER}/thirdparty/toolcursor"
)

# ------------------------------------------------------------------------------
# Link libraries
# ------------------------------------------------------------------------------
find_package(Qt5 REQUIRED COMPONENTS Widgets)
target_link_libraries(${LIB_NAME} PRIVATE Qt5::Widgets)
target_link_libraries(${LIB_NAME} PRIVATE ${LIB_DEPS})

# ------------------------------------------------------------------------------
# Export for parent scopes
# ------------------------------------------------------------------------------
set(Cerebra_LIBRARIES
    ${Cerebra_LIBRARIES} ${PROJECT_NAME}
    PARENT_SCOPE)

set(INCLUDE_OUTPUT_PATH
    ${INCLUDE_OUTPUT_PATH} ${CMAKE_CURRENT_SOURCE_DIR}
    PARENT_SCOPE)

export(
  TARGETS ${LIB_NAME}
  APPEND
  FILE "${CONFIGURE_OUTPUT_PATH}/CerebraTargets.cmake"
)
