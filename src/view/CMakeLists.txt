cmake_minimum_required(VERSION 3.16)

project(cbView VERSION 1.0 LANGUAGES CXX)

set(LIB_NAME ${PROJECT_NAME})

# --------------------------------------------------------------
# 1. Sources
# --------------------------------------------------------------
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

# --------------------------------------------------------------
# 2. Find VTK
# --------------------------------------------------------------
find_package(VTK 9.5 REQUIRED COMPONENTS
  CommonCore
  CommonDataModel
  CommonExecutionModel
  FiltersCore
  ImagingCore
  RenderingCore
  IOLegacy
)

# --------------------------------------------------------------
# 3. Find or link ToolCursor
# --------------------------------------------------------------
find_package(ToolCursor REQUIRED)


# --------------------------------------------------------------
# 4. Library
# --------------------------------------------------------------
add_library(${LIB_NAME} ${LIB_SRCS})

target_include_directories(${LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/cbView>
        /usr/local/include/vtk-9.5
)

target_link_libraries(${LIB_NAME}
    PUBLIC
        VTK::CommonCore
        VTK::CommonDataModel
        VTK::CommonExecutionModel
        VTK::FiltersCore
        VTK::ImagingCore
        VTK::RenderingCore
        VTK::IOLegacy
        VTK::ToolCursor
)

# --------------------------------------------------------------
# 5. Install + export
# --------------------------------------------------------------
install(TARGETS ${LIB_NAME}
    EXPORT cbViewTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(EXPORT cbViewTargets
    FILE cbViewTargets.cmake
    NAMESPACE cbView::
    DESTINATION lib/cmake/cbView
)
