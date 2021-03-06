PROJECT(AIRS)

# CMake version requirements
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# For CMake 3.0, prefer rpath over loader_path on OS X
IF(POLICY CMP0042)
  CMAKE_POLICY(SET CMP0042 NEW)
ENDIF(POLICY CMP0042)

#SET(CMAKE_SKIP_RPATH ON CACHE BOOL "rpath not included in .so files" FORCE)

# Look for a VTK build directory
FIND_PACKAGE(VTK)
IF (VTK_FOUND)
  INCLUDE (${VTK_USE_FILE})
ENDIF (VTK_FOUND)

# Look for DICOM add-ons to VTK
OPTION(AIRS_USE_DICOM "Use DICOM in AIRS" OFF)
IF(AIRS_USE_DICOM)
  FIND_PACKAGE(DICOM QUIET)
  IF(DICOM_FOUND)
    INCLUDE(${DICOM_USE_FILE})
  ENDIF(DICOM_FOUND)
  # GDCM is required for reading compressed DICOM images
  OPTION(USE_GDCM "Use GDCM for image decompression" OFF)
  IF(USE_GDCM)
    FIND_PACKAGE(GDCM)
    IF(NOT GDCM_DIR)
      MESSAGE(FATAL_ERROR "Please set GDCM_DIR.")
    ENDIF(NOT GDCM_DIR)
    IF(GDCM_FOUND)
      INCLUDE(${GDCM_USE_FILE})
      SET(GDCM_LIBS gdcmMSFF gdcmIOD gdcmDSED gdcmDICT)
    ENDIF(GDCM_FOUND)
  ENDIF(USE_GDCM)
ENDIF(AIRS_USE_DICOM)

# Choose static or shared libraries.
OPTION(BUILD_SHARED_LIBS "Build shared libraries." OFF)
SET (AIRS_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
IF (BUILD_SHARED_LIBS)
  SET (BUILD_SHARED_LIBS_CONFIG 1)
ELSE (BUILD_SHARED_LIBS)
  SET (BUILD_SHARED_LIBS_CONFIG 0)
ENDIF (BUILD_SHARED_LIBS)

# Build documentation
OPTION(BUILD_DOCUMENTATION "Build Documentation" OFF)
IF(BUILD_DOCUMENTATION)
  FIND_PACKAGE(VTK)
  IF (USE_VTK_FILE)
    INCLUDE (${USE_VTK_FILE})
  ELSE (USE_VTK_FILE)
    MESSAGE(FATAL_ERROR "This application requires VTK.")
  ENDIF (USE_VTK_FILE)
  ADD_SUBDIRECTORY(Documentation/Doxygen)
ENDIF(BUILD_DOCUMENTATION)

# Output directories.
SET(LIBRARY_OUTPUT_PATH ${AIRS_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all libraries.")
SET(EXECUTABLE_OUTPUT_PATH ${AIRS_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all executables.")
SET(AIRS_LIBRARY_DIR ${LIBRARY_OUTPUT_PATH}/${CMAKE_CFG_INTDIR})
SET(AIRS_EXECUTABLE_DIR ${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR})
SET(CXX_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})

# Configure Dart testing support.
INCLUDE(${CMAKE_ROOT}/Modules/Dart.cmake)

# ---------------------------------------------------------------
# Initialize configuration

# Kits
SET(AIRS_KITS)

# Libraries
SET(AIRS_LIBRARIES)

# Include directories
SET(AIRS_INCLUDE_DIRS ${AIRS_BINARY_DIR})

# ---------------------------------------------------------------
# Include the Tcl and Python wrappers
IF(VTK_WRAP_TCL)
  IF(VTK_TCL_INCLUDE_DIR)
    INCLUDE_DIRECTORIES("${VTK_TCL_INCLUDE_DIR}")
  ENDIF(VTK_TCL_INCLUDE_DIR)
  INCLUDE(${VTK_CMAKE_DIR}/vtkWrapTcl.cmake)
ENDIF(VTK_WRAP_TCL)

IF(VTK_WRAP_PYTHON)
  IF(VTK_PYTHON_INCLUDE_DIR)
    INCLUDE_DIRECTORIES("${VTK_PYTHON_INCLUDE_DIR}")
  ENDIF(VTK_PYTHON_INCLUDE_DIR)
  INCLUDE(${VTK_CMAKE_DIR}/vtkWrapPython.cmake)
ENDIF(VTK_WRAP_PYTHON)

IF(VTK_WRAP_JAVA)
  SET(VTK_WRAP_JAVA 0)
ENDIF(VTK_WRAP_JAVA)

# Include AIRSThirdParty
INCLUDE(${AIRS_SOURCE_DIR}/CMake/AIRSThirdParty.cmake)

# ---------------------------------------------------------------
# Start listing the AIRS kits

# ImageRegistration
OPTION (AIRS_USE_IMAGEREGISTRATION "Build the ImageRegistration module" ON)
IF (AIRS_USE_IMAGEREGISTRATION)
  SET(AIRS_INCLUDE_DIRS ${AIRS_INCLUDE_DIRS}
      "${AIRS_BINARY_DIR}/ImageRegistration"
      "${AIRS_SOURCE_DIR}/ImageRegistration")
  ADD_SUBDIRECTORY(ImageRegistration)
  SET(AIRS_LIBRARIES ${AIRS_LIBRARIES} vtkImageRegistration)
ENDIF (AIRS_USE_IMAGEREGISTRATION)

# ImageSegmentation
OPTION (AIRS_USE_IMAGESEGMENTATION "Build the ImageSegmentation module" ON)
IF (AIRS_USE_IMAGESEGMENTATION)
  SET(AIRS_INCLUDE_DIRS ${AIRS_INCLUDE_DIRS}
      "${AIRS_BINARY_DIR}/ImageSegmentation"
      "${AIRS_SOURCE_DIR}/ImageSegmentation")
  ADD_SUBDIRECTORY(ImageSegmentation)
  SET(AIRS_LIBRARIES ${AIRS_LIBRARIES} vtkImageSegmentation)
ENDIF (AIRS_USE_IMAGESEGMENTATION)

# Build Programs
OPTION(BUILD_PROGRAMS "Build standard programs" ON)
IF(BUILD_PROGRAMS)
   ADD_SUBDIRECTORY(Programs)
ENDIF(BUILD_PROGRAMS)

# Build Testing
OPTION(BUILD_TESTING "Build the tests" OFF)
IF(BUILD_TESTING)
   ADD_SUBDIRECTORY(Testing)
ENDIF(BUILD_TESTING)

# Build Examples
OPTION(BUILD_EXAMPLES "Build the examples" ON)
IF (BUILD_EXAMPLES)
   ADD_SUBDIRECTORY(Examples)
ENDIF (BUILD_EXAMPLES)

# Add airs.py.in file.  On Win32, this has to go into all config
IF(VTK_WRAP_PYTHON)
  IF(WIN32 AND CMAKE_CONFIGURATION_TYPES)
    FOREACH(config ${CMAKE_CONFIGURATION_TYPES})
      CONFIGURE_FILE(
        ${AIRS_SOURCE_DIR}/airs.py.in
        ${LIBRARY_OUTPUT_PATH}/${config}/airs.py)
    ENDFOREACH(config)
  ELSE(WIN32 AND CMAKE_CONFIGURATION_TYPES)
    CONFIGURE_FILE(
      ${AIRS_SOURCE_DIR}/airs.py.in
      ${LIBRARY_OUTPUT_PATH}/airs.py)
  ENDIF(WIN32 AND CMAKE_CONFIGURATION_TYPES)
ENDIF(VTK_WRAP_PYTHON)

# Configuration header
SET(AIRS_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
SET(AIRS_BUILD_TESTING ${BUILD_TESTING})
CONFIGURE_FILE(CMake/AIRSConfig.h.in
  "${AIRS_BINARY_DIR}/AIRSConfig.h" @ONLY)

#-----------------------------------------------------------------------------
# The commands in this directory are intended to be executed as
# the end of the whole configuration process, as a "last step".
# This directory is typically the last SUBDIRS in the main CMakeLists.txt.
ADD_SUBDIRECTORY(CMake/LastConfigureStep)
