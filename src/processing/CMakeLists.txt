project(cbProcessing)

set(Cerebra_LIBRARIES ${Cerebra_LIBRARIES} ${PROJECT_NAME} PARENT_SCOPE)
set(INCLUDE_OUTPUT_PATH ${INCLUDE_OUTPUT_PATH} ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

cmake_minimum_required(VERSION 3.16)

# Mandatory policy setting
if(COMMAND CMAKE_POLICY)
  cmake_policy(SET CMP0003 NEW)
endif()

# The library name, dependencies, and source files
set(LIB_NAME ${PROJECT_NAME})
set(LIB_DEPS
  vtkImaging
  vtkCommon
  vtkImageRegistration
  cbVTK
)

set(LIB_SRCS
  cbMRIRegistration.cxx
)

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${MAINFOLDER}/thirdparty/airs/ImageRegistration/"
  "${MAINFOLDER}/thirdparty/upstream/vtk/"
  "${MAINFOLDER}/thirdparty/upstream/itk/"
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

export(TARGETS ${LIB_NAME}
  APPEND FILE "${CONFIGURE_OUTPUT_PATH}/CerebraTargets.cmake")
