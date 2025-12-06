project(cbDICOM)

set(Cerebra_LIBRARIES ${Cerebra_LIBRARIES} ${PROJECT_NAME} PARENT_SCOPE)
set(INCLUDE_OUTPUT_PATH ${INCLUDE_OUTPUT_PATH} ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

cmake_minimum_required(VERSION 3.16)

# Mandatory policy setting
if(COMMAND CMAKE_POLICY)
  cmake_policy(SET CMP0003 NEW)
endif()

# Project version
set(DICOM_MAJOR_VERSION 0)
set(DICOM_MINOR_VERSION 7)
set(DICOM_PATCH_VERSION 7)
set(DICOM_SHORT_VERSION "${DICOM_MAJOR_VERSION}.${DICOM_MINOR_VERSION}")
set(DICOM_VERSION "${DICOM_SHORT_VERSION}.${DICOM_PATCH_VERSION}")

# Configuration header
set(DICOM_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(DICOM_BUILD_TESTING ${BUILD_TESTING})
set(DICOM_USE_GDCM ${USE_GDCM})
set(DICOM_USE_DCMTK ${USE_DCMTK})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkDICOMConfig.h.in
  "${CMAKE_CURRENT_BINARY_DIR}/vtkDICOMConfig.h" @ONLY)

# Include directories
set(DICOM_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${DICOM_INCLUDE_DIRS})

# The library name, dependencies, and source files
set(LIB_NAME ${PROJECT_NAME})
set(LIB_DEPS vtkIO vtkzlib)

if(WIN32)
  set(REFCOUNT_SRC vtkDICOMReferenceCount.cxx)
else()
  set(REFCOUNT_HDR vtkDICOMReferenceCount.h)
endif()

# Sources in the current directory (library sources only!)
set(LIB_SRCS
  vtkDICOMMetaData.cxx
  vtkDICOMDictionary.cxx
  vtkDICOMFilePath.cxx
  vtkDICOMFile.cxx
  vtkDICOMFileDirectory.cxx
  vtkDICOMTag.cxx
  vtkDICOMTagPath.cxx
  vtkDICOMVR.cxx
  vtkDICOMVM.cxx
  vtkDICOMCharacterSet.cxx
  vtkDICOMDataElement.cxx
  vtkDICOMDictHash.cxx
  vtkDICOMDictEntry.cxx
  vtkDICOMDictPrivate.cxx
  vtkDICOMDirectory.cxx
  vtkDICOMFileSorter.cxx
  vtkDICOMGenerator.cxx
  vtkDICOMImageCodec.cxx
  vtkDICOMSCGenerator.cxx
  vtkDICOMCTGenerator.cxx
  vtkDICOMMRGenerator.cxx
  vtkDICOMParser.cxx
  vtkDICOMCompiler.cxx
  vtkDICOMReader.cxx
  vtkDICOMSliceSorter.cxx
  vtkDICOMSequence.cxx
  vtkDICOMItem.cxx
  vtkDICOMSorter.cxx
  ${REFCOUNT_SRC}
  vtkDICOMUtilities.cxx
  vtkDICOMUtilitiesUIDTable.cxx
  vtkDICOMValue.cxx
  vtkDICOMWriter.cxx
  vtkDICOMAlgorithm.cxx
  vtkDICOMLookupTable.cxx
  vtkDICOMApplyPalette.cxx
  vtkDICOMApplyRescale.cxx
  vtkDICOMToRAS.cxx
  vtkDICOMCTRectifier.cxx
  vtkDICOMMetaDataAdapter.cxx
  vtkNIFTIHeader.cxx
  vtkNIFTIReader.cxx
  vtkNIFTIWriter.cxx
)

# Headers without a matching .cxx file are listed here
set(LIB_HDRS
  ${REFCOUNT_HDR}
  vtkNIFTIPrivate.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkDICOMBuild.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkDICOMConfig.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkDICOMModule.h
)

# Internal headers (not installed)
set(LIB_PRIVATE_HDRS
  vtkNIFTIPrivate.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkDICOMBuild.h
)

set_source_files_properties(${LIB_PRIVATE_HDRS}
  PROPERTIES SKIP_HEADER_INSTALL ON)

# Let CMake know which sources have .h without a .cxx
set_source_files_properties(${LIB_HDRS} HEADER_FILE_ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkDICOMModule.h.in
  "${CMAKE_CURRENT_BINARY_DIR}/vtkDICOMModule.h" COPYONLY)

# Automatically find the header for each cxx file
foreach(arg ${LIB_SRCS})
  get_filename_component(src "${arg}" ABSOLUTE)
  string(REGEX REPLACE "\\.(cxx|c|mm|m)$" ".h" hdr "${src}")
  if("${hdr}" MATCHES "\\.h$" AND EXISTS "${hdr}")
    list(APPEND LIB_HDRS "${hdr}")
  endif()
endforeach()

# Create the library
add_library(${LIB_NAME} STATIC ${LIB_SRCS})
target_link_libraries(${LIB_NAME} ${LIB_DEPS})

export(TARGETS ${LIB_NAME}
  APPEND FILE "${CONFIGURE_OUTPUT_PATH}/CerebraTargets.cmake")
