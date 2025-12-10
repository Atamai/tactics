cmake_minimum_required(VERSION 3.16)
project(cbData)

# ---- Find VTK ----
find_package(VTK 9.5 REQUIRED
  COMPONENTS
    CommonCore
    CommonDataModel
    FiltersCore
    FiltersGeneral
)

find_package(dicom REQUIRED)

# ---- Library ----
set(LIB_NAME ${PROJECT_NAME})

set(LIB_SRCS
  vtkDataManager.cxx
  vtkDataManagerDefaultKeys.cxx
  vtkDataNode.cxx
  vtkFrameOfReference.cxx
  vtkImageNode.cxx
  vtkSurfaceNode.cxx
)

add_library(${LIB_NAME} ${LIB_SRCS})

# ---- Include dirs for your own code ----
target_include_directories(${LIB_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ---- Link correct VTK modules ----
target_link_libraries(${LIB_NAME}
  PRIVATE
    VTK::CommonCore
    VTK::CommonDataModel
    VTK::FiltersCore
    VTK::FiltersGeneral
)

# ---- Export ----
export(
  TARGETS ${LIB_NAME}
  APPEND FILE "${CONFIGURE_OUTPUT_PATH}/CerebraTargets.cmake"
)
