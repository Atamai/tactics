project(cbITK)

# CMake version requirements
cmake_minimum_required(VERSION 2.8)

# Mandatory policy setting
if(COMMAND CMAKE_POLICY)
  cmake_policy(SET CMP0003 NEW)
endif()

# Find ITK.
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

# The library name, dependencies, and source files
set(LIB_NAME cbITK)
set(LIB_DEPS ${ITK_LIBRARIES})
set(LIB_SRCS
  itkProgressAccumulator2.cxx
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Replacement/ITK
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
