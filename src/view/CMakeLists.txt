cmake_minimum_required(VERSION 3.16)

project(cbView LANGUAGES CXX)

set(LIB_NAME ${PROJECT_NAME})

# 1. Sources
set(LIB_SRCS
  vtkViewObject.cxx
  vtkViewObjectCollection.cxx
  vtkViewRect.cxx
  vtkViewFrame.cxx
  vtkViewPane.cxx
  vtkDummyViewPane.cxx
  vtkImageViewPane.cxx
  vtkDynamicViewFrame.cxx
  vtkHViewFrame.cxx
  vtkVViewFrame.cxx
  vtkGViewFrame.cxx
  vtkViewToolCursorInteractorObserver.cxx
)

# 2. Find VTK
find_package(VTK 9.5 REQUIRED COMPONENTS
  CommonCore
  CommonDataModel
  CommonExecutionModel
  FiltersCore
  ImagingCore
  RenderingCore
  IOLegacy
)

# 3. Library
add_library(${LIB_NAME} ${LIB_SRCS})

target_include_directories(${LIB_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${MAINFOLDER}/thirdparty/toolcursor/"
)

target_link_libraries(${LIB_NAME} PUBLIC
  VTK::CommonCore
  VTK::CommonDataModel
  VTK::CommonExecutionModel
  VTK::FiltersCore
  VTK::ImagingCore
  VTK::RenderingCore
  VTK::IOLegacy
)

# 4. Export for external use
export(TARGETS ${LIB_NAME}
    APPEND FILE "${CONFIGURE_OUTPUT_PATH}/CerebraTargets.cmake"
)
