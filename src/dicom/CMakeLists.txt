project(cbDICOM)

set(Cerebra_LIBRARIES ${Cerebra_LIBRARIES} ${PROJECT_NAME} PARENT_SCOPE)
set(INCLUDE_OUTPUT_PATH ${INCLUDE_OUTPUT_PATH} ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

# CMake version requirements
cmake_minimum_required(VERSION 2.8)

# Mandatory policy setting
if(COMMAND CMAKE_POLICY)
  cmake_policy(SET CMP0003 NEW)
endif()

# Project version
set(DICOM_MAJOR_VERSION 0)
set(DICOM_MINOR_VERSION 1)
set(DICOM_BUILD_VERSION 0)
set(DICOM_SHORT_VERSION "${DICOM_MAJOR_VERSION}.${DICOM_MINOR_VERSION}")
set(DICOM_VERSION "${DICOM_SHORT_VERSION}.${DICOM_BUILD_VERSION}")

# The library name, dependencies, and source files
# Use ITK to get gdcm
set(LIB_NAME ${PROJECT_NAME})
set(LIB_DEPS vtkIO vtkzlib ${ITK_LIBRARIES})

# Sources in the current directory (library sources only!)
set(LIB_SRCS
  vtkDICOMMetaData.cxx
  vtkDICOMDictionary.cxx
  vtkDICOMTag.cxx
  vtkDICOMVR.cxx
  vtkDICOMVM.cxx
  vtkDICOMDataElement.cxx
  vtkDICOMDictHash.cxx
  vtkDICOMDictEntry.cxx
  vtkDICOMParser.cxx
  vtkDICOMReader.cxx
  vtkDICOMSequence.cxx
  vtkDICOMItem.cxx
  vtkDICOMSorter.cxx
  vtkDICOMReferenceCount.cxx
  vtkDICOMValue.cxx
  vtkDICOMToRAS.cxx
  vtkNIFTIReader.cxx
  vtkNIFTIWriter.cxx
)

# Headers without a matching .cxx file are listed here
set(LIB_HDRS
  vtkDICOMModule.h
  vtkNIFTIHeader.h
  ${DICOM_BINARY_DIR}/vtkDICOMConfig.h
)

set(DICOM_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${DICOM_INCLUDE_DIRS})

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
add_library(${LIB_NAME} STATIC ${LIB_SRCS})
target_link_libraries(${LIB_NAME} ${LIB_DEPS})

export(TARGETS ${LIB_NAME}
  APPEND FILE "${EXECUTABLE_OUTPUT_PATH}/CerebraTargets.cmake")
