PROJECT (cbProcessing)

SET (Cerebra_LIBRARIES ${Cerebra_LIBRARIES} ${PROJECT_NAME} PARENT_SCOPE)
SET (INCLUDE_OUTPUT_PATH ${INCLUDE_OUTPUT_PATH} ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

# CMake version requirements
CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

# Mandatory policy setting
IF (COMMAND CMAKE_POLICY)
  cmake_policy(SET CMP0003 NEW)
ENDIF()

# The library name, dependencies, and source files
SET (LIB_NAME ${PROJECT_NAME})
SET (LIB_DEPS
  ${ITK_LIBRARIES}
  vtkImaging
  vtkCommon
  ItkVtkGlue
  cbAIRS
  cbVTK
  cbITK
)

SET (LIB_SRCS
  cbMRIRegistration.cxx
)

INCLUDE_DIRECTORIES (
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${MAINFOLDER}/thirdparty/airs/ImageRegistration/"
  "${MAINFOLDER}/thirdparty/upstream/vtk/"
  "${MAINFOLDER}/thirdparty/upstream/itk/"
)

# Create a list of header files (source files that end in .h)
SET (LIB_HDRS)
FOREACH (source_file ${LIB_SRCS})
  GET_FILENAME_COMPONENT (file_extension ${source_file} EXT)
  IF ("${file_extension}" STREQUAL ".h")
    SET (LIB_HDRS ${LIB_HDRS} ${source_file})
  ENDIF()
ENDFOREACH()

# Let CMake know which sources have .h without a .cxx
IF (LIB_HDRS)
  SET_SOURCE_FILES_PROPERTIES (${LIB_HDRS} HEADER_FILE_ONLY)
ENDIF()

# Create the library
ADD_LIBRARY (${LIB_NAME} ${LIB_SRCS})
TARGET_LINK_LIBRARIES (${LIB_NAME} ${LIB_DEPS})

EXPORT (TARGETS ${LIB_NAME}
  APPEND FILE "${EXECUTABLE_OUTPUT_PATH}/CerebraTargets.cmake")
