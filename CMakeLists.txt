# David Adair (Calgary Image Processing and Analysis Centre).
# Copyright (C) 2011-2013. BSD(3) License.

#
# Project Properties
#
cmake_minimum_required(VERSION 2.6.2)
project(tactics)
set(APPLICATION_NAME "Tactics")
set(APPLICATION_CODENAME "${PROJECT_NAME}")
set(APPLICATION_COPYRIGHT_YEARS "2013")
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

#
# Project Search Paths
#
list(APPEND CMAKE_PREFIX_PATH "${MAINFOLDER}")
list(APPEND CMAKE_PREFIX_PATH "${MAINFOLDER}/tools")
list(APPEND CMAKE_PREFIX_PATH "${MAINFOLDER}/thirdparty")
set(CMAKE_MODULE_PATH "${MAINFOLDER}/tools/share/cmake")
include_directories("${MAINFOLDER}/src")

find_package(Qt4 REQUIRED)
include(UseQt4)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

find_package(Cerebra REQUIRED)
#INCLUDE(${Cerebra_USE_FILE})
include_directories(${Cerebra_INCLUDE_DIRS})
link_directories(${Cerebra_LIBRARY_DIRS})

find_package(UnitTestPlusPlus REQUIRED)
include_directories(${UnitTestPlusPlus_INCLUDE_DIRS})
link_directories(${UnitTestPlusPlus_LIBRARY_DIRS})

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
add_subdirectory(src)
#ADD_SUBDIRECTORY(test)

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
