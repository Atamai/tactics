# David Adair. Copyright (C) 2012-2013. BSD(3).

#
# Project Properties
#
cmake_minimum_required(VERSION 2.6.2)
project(cerebra)
set(APPLICATION_NAME "Cerebra")
set(APPLICATION_CODENAME "${PROJECT_NAME}")
set(APPLICATION_COPYRIGHT_YEARS "2012-2013")
set(APPLICATION_VERSION_MAJOR 1)
set(APPLICATION_VERSION_MINOR 0)
set(APPLICATION_VERSION_PATCH 0)
set(APPLICATION_VERSION_TYPE SNAPSHOT)
set(APPLICATION_VERSION_STRING "${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}.${APPLICATION_VERSION_PATCH}-${APPLICATION_VERSION_TYPE}")
set(APPLICATION_VENDOR_ID "ca.calgaryimageanalysis")
set(APPLICATION_VENDOR_NAME "Calgary Image Processing and Analysis Centre")
set(APPLICATION_VENDOR_URL "calgaryimageanalysis.ca")
set(APPLICATION_ID "${APPLICATION_VENDOR_ID}.${PROJECT_NAME}")

#
# Debugging Options
#
set(CMAKE_VERBOSE_MAKEFILE 0) # Use 1 for debugging, 0 for release

#
# Project Output Paths
#
set(MAINFOLDER ${PROJECT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH "${MAINFOLDER}/bin")
set(LIBRARY_OUTPUT_PATH "${MAINFOLDER}/lib")
set(CONFIGURE_OUTPUT_PATH "${MAINFOLDER}/build")

file(MAKE_DIRECTORY
 "${EXECUTABLE_OUTPUT_PATH}"
 "${LIBRARY_OUTPUT_PATH}"
 "${CONFIGURE_OUTPUT_PATH}")

#
# FIXME: These directories should append themselves to the list
# (apart from thirdparty applications that are compiled into libraries)
#
set(INCLUDE_OUTPUT_PATH
  "${MAINFOLDER}/include"
  "${MAINFOLDER}/thirdparty/toolcursor"
  "${MAINFOLDER}/thirdparty/airs/ImageRegistration/"
  "${MAINFOLDER}/thirdparty/airs/ImageSegmentation/"
  "${MAINFOLDER}/thirdparty/upstream/vtk/"
  "${MAINFOLDER}/thirdparty/upstream/airs/"
  )

#
# Libraries built
#
set(Cerebra_LIBRARIES "vtkToolCursor" ${VTK_LIBRARIES} ${QT_LIBRARIES})

#
# Project Search Paths
#
list(APPEND CMAKE_PREFIX_PATH "${MAINFOLDER}")
list(APPEND CMAKE_PREFIX_PATH "${MAINFOLDER}/tools")
list(APPEND CMAKE_PREFIX_PATH "${MAINFOLDER}/thirdparty")
set(CMAKE_MODULE_PATH "${MAINFOLDER}/tools/share/cmake")
include_directories(${INCLUDE_OUTPUT_PATH})

#
# Locate Project Prerequisites
#
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

# Find ITK_VTK_GLUE
find_package(ItkVtkGlue REQUIRED)
include(${ItkVtkGlue_USE_FILE})

find_package(Qt4 REQUIRED)
include(UseQt4)

#
# Configure Files
#
file(GLOB_RECURSE CONFIGINPUTS1 include/*.in.h.cmake)
file(GLOB_RECURSE CONFIGINPUTS2 include/*.h.in.cmake)
file(GLOB_RECURSE CONFIGINPUTS3 src/*.in.h.cmake)
file(GLOB_RECURSE CONFIGINPUTS4 src/*.h.in.cmake)
file(GLOB_RECURSE CONFIGINPUTS5 test/*.in.h.cmake)
file(GLOB_RECURSE CONFIGINPUTS6 test/*.h.in.cmake)
list(APPEND CONFIGINPUTS "${CONFIGINPUTS1}")
list(APPEND CONFIGINPUTS "${CONFIGINPUTS2}")
list(APPEND CONFIGINPUTS "${CONFIGINPUTS3}")
list(APPEND CONFIGINPUTS "${CONFIGINPUTS4}")
list(APPEND CONFIGINPUTS "${CONFIGINPUTS5}")
list(APPEND CONFIGINPUTS "${CONFIGINPUTS6}")
foreach(CONFIGINPUT ${CONFIGINPUTS})
  string(REPLACE ".in.h.cmake" ".h" CONFIGOUTPUT1 "${CONFIGINPUT}")
  string(REPLACE ".h.in.cmake" ".h" CONFIGOUTPUT2 "${CONFIGOUTPUT1}")
  configure_file("${CONFIGINPUT}" "${CONFIGOUTPUT2}")
endforeach(CONFIGINPUT ${CONFIGINPUTS})

#
# Add Build Targets
#
add_subdirectory(src/gui)
add_subdirectory(src/data)
add_subdirectory(src/view)
add_subdirectory(src/dicom)
add_subdirectory(src/processing)
add_subdirectory(test)
add_subdirectory(thirdparty)

#
# Add Install Targets
#
if(EXISTS "${MAINFOLDER}/include/${PROJECT_NAME}" AND IS_DIRECTORY "${MAINFOLDER}/include/${PROJECT_NAME}")
  install(DIRECTORY "${MAINFOLDER}/include/${PROJECT_NAME}" DESTINATION "include")
endif(EXISTS "${MAINFOLDER}/include/${PROJECT_NAME}" AND IS_DIRECTORY "${MAINFOLDER}/include/${PROJECT_NAME}")

#
# Add Documentation Targets
#
include(DocumentationTargets)

#
# Export CMAKE Variables for external FIND_PACKAGE(..) calls
#
configure_file(cmake/CerebraConfig.cmake.in
  "${CONFIGURE_OUTPUT_PATH}/CerebraConfig.cmake" @ONLY)
configure_file(cmake/UseCerebra.cmake.in
  "${CONFIGURE_OUTPUT_PATH}/UseCerebra.cmake" COPYONLY IMMEDIATE)

#message(STATUS "includes: ${INCLUDE_OUTPUT_PATH}")
#message(STATUS "libraries: ${Cerebra_LIBRARIES}")

export(PACKAGE Cerebra)
