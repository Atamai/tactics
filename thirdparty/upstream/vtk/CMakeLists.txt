cmake_minimum_required(VERSION 3.16)

# 1. Project definition
project(cbVTK LANGUAGES CXX)

# 2. VTK 9.5 requires C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 3. Find VTK with specific components
# vtkCommon -> VTK::CommonCore, VTK::CommonDataModel
# vtkFiltering -> VTK::FiltersCore, VTK::CommonExecutionModel
# vtkImaging -> VTK::ImagingCore
# Reader support -> VTK::IOLegacy (likely needed for vtkMNITagPointReader2)
find_package(VTK 9.5 REQUIRED COMPONENTS
  CommonCore
  CommonDataModel
  CommonExecutionModel
  FiltersCore
  ImagingCore
  IOLegacy
)

# 4. Define Sources
set(LIB_SRCS
  vtkContourToPolygonFilter.cxx
  vtkMNITagPointReader2.cxx
  vtkProgressAccumulator.cxx
  # You can list headers here if you want them to show up in IDEs
  # vtkContourToPolygonFilter.h 
)

# 5. Create the Library
add_library(cbVTK ${LIB_SRCS})

# 6. Link Libraries (Using modern namespaced targets)
# We use PUBLIC so that projects linking to cbVTK automatically
# get the VTK include paths and link flags.
target_link_libraries(cbVTK PUBLIC
  VTK::CommonCore
  VTK::CommonDataModel
  VTK::CommonExecutionModel
  VTK::FiltersCore
  VTK::ImagingCore
  VTK::IOLegacy
)

# 7. Include Directories
target_include_directories(cbVTK PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)