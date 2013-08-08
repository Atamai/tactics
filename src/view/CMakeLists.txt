project(cbViews)

# CMake version requirements
cmake_minimum_required(VERSION 2.8)

# Mandatory policy setting
if(COMMAND CMAKE_POLICY)
  cmake_policy(SET CMP0003 NEW)
endif()

find_library(TOOL_CURSOR_LIB vtkToolCursor)

# The library name, dependencies, and source files
set(LIB_NAME ${PROJECT_NAME})
set(LIB_DEPS vtkHybrid vtkToolCursor vtkVolumeRendering)

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

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${MAINFOLDER}/thirdparty/toolcursor/"
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
