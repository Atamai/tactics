project(cbVTK)

# CMake version requirements
cmake_minimum_required(VERSION 2.8)

# Mandatory policy setting
if(COMMAND CMAKE_POLICY)
  cmake_policy(SET CMP0003 NEW)
endif()

# Find VTK
if(NOT VTK_FOUND)
  find_package(VTK)
  if(VTK_FOUND)
    include(${VTK_USE_FILE})
  endif()
endif()

# The library name, dependencies, and source files
set(LIB_NAME cbVTK)
set(LIB_DEPS vtkCommon vtkImaging vtkFiltering vtkViews vtkInfovis vtkHybrid)
set(LIB_SRCS
  vtkBitArray2.cxx
  vtkContourToPolygonFilter.cxx
  vtkCornerAnnotation2.cxx
  vtkProgressAccumulator.cxx
  vtkImageGaussianInterpolator.cxx
  vtkImageThresholdConnectivity2.cxx
  vtkImageAutoCrop.cxx
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Replacement/VTK
)

# Create a list of header files (source files that end in .h)
set(LIB_HDRS)
foreach(source_file ${LIB_SRCS})
  get_filename_component(file_extension ${source_file} EXT)
  if("${file_extension}" STREQUAL ".h")
    set(LIB_HDRS ${LIB_HDRS} ${source_file})
  endif()
endforeach()

# Let CMake know which sources have .h without a .cxx
if(LIB_HDRS)
  set_source_files_properties(${LIB_HDRS} HEADER_FILE_ONLY)
endif()

# Create the library
add_library(${LIB_NAME} ${LIB_SRCS})
target_link_libraries(${LIB_NAME} ${LIB_DEPS})
