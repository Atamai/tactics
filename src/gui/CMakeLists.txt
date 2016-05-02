project(cbGUI)

set(Cerebra_LIBRARIES ${Cerebra_LIBRARIES} ${PROJECT_NAME} PARENT_SCOPE)
set(INCLUDE_OUTPUT_PATH ${INCLUDE_OUTPUT_PATH} ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

# CMake version requirements
cmake_minimum_required(VERSION 2.8.10)

if(POLICY CMP0043)
  cmake_policy(SET CMP0043 OLD)
endif()

# The library name, dependencies, and source files
set(LIB_NAME ${PROJECT_NAME})
set(LIB_DEPS
  vtkCommon
  vtkRendering
  vtkToolCursor
  cbView
  cbData
  vtkToolCursor
  cbVTK
  )

set(LIB_SRCS
  qvtkViewToolCursorWidget.cxx
  cbMainWindow.cxx
  cbApplicationController.cxx
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

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${MAINFOLDER}/src/view"
  "${MAINFOLDER}/src/data"
  "${MAINFOLDER}/thirdparty/toolcursor"
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
qt5_use_modules(${LIB_NAME} Widgets)
target_link_libraries(${LIB_NAME} ${LIB_DEPS})

export(TARGETS ${LIB_NAME}
  APPEND FILE "${CONFIGURE_OUTPUT_PATH}/CerebraTargets.cmake")
