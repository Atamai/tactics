# David Adair (Calgary Image Processing and Analysis Centre).
# Copyright (C) 2011-2013. BSD(3) License.

#
# Project Properties
#
cmake_minimum_required(VERSION 2.6.2)
project(Tactics)
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
set(CONFIGURE_OUTPUT_PATH "${MAINFOLDER}/build")

file(MAKE_DIRECTORY
 "${EXECUTABLE_OUTPUT_PATH}"
 "${LIBRARY_OUTPUT_PATH}"
 "${CONFIGURE_OUTPUT_PATH}")

#
# Project Data Path
#
set(APPLICATION_DATA_PATH "${MAINFOLDER}/share")
set(APPLICATION_ICON_PATH
  "${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.app/Contents/Icons")
set(APPLICATION_RESOURCE_PATH
  "${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.app/Contents/Resources")

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

include_directories(${INCLUDE_OUTPUT_PATH})
include_directories("${MAINFOLDER}/src/view")
include_directories("${MAINFOLDER}/src/data")
include_directories("${MAINFOLDER}/src/gui")
include_directories("${MAINFOLDER}/src/processing")
include_directories("${MAINFOLDER}/src/dicom")
include_directories("${MAINFOLDER}/build/src/dicom")
include_directories("${MAINFOLDER}/src/tactics")

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

#
# Locate Project Prerequisites
#
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

find_package(Qt4 REQUIRED)
include(UseQt4)

#
# Application icon
#
set(TACTICS_ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/resources/Tactics.icns)

#
# Add Build Targets
#
add_subdirectory(thirdparty)
add_subdirectory(src/gui)
add_subdirectory(src/data)
add_subdirectory(src/view)
add_subdirectory(src/dicom)
add_subdirectory(src/processing)
add_subdirectory(src/tactics)
add_subdirectory(test)

#
# Copy icons
#
foreach(ICONTYPE Icons Cursors)
  string(TOLOWER "${ICONTYPE}" LC_ICONTYPE)
  file(GLOB_RECURSE ICON_FILES
       RELATIVE ${CMAKE_SOURCE_DIR}/resources/${LC_ICONTYPE}
       resources/${LC_ICONTYPE}/*.png)
  set(ALL_ICON_FILES)
  foreach(ICON_FILE ${ICON_FILES})
    set(src ${CMAKE_SOURCE_DIR}/resources/${LC_ICONTYPE}/${ICON_FILE})
    set(tgt ${APPLICATION_ICON_PATH}/${ICONTYPE}/${ICON_FILE})
    add_custom_command(DEPENDS ${src}
                       COMMAND ${CMAKE_COMMAND} -E copy ${src} ${tgt}
                       OUTPUT ${tgt}
                       COMMENT "icon copy ${LC_ICONTYPE}/${ICON_FILE}")
    set(ALL_ICON_FILES ${ALL_ICON_FILES} ${tgt})
  endforeach()

  add_custom_target(Copy${ICONTYPE} ALL echo "..."
    DEPENDS ${ALL_ICON_FILES})

  install(FILES ${ALL_ICON_FILES}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Icons/${ICONTYPE})
endforeach()

#
# Copy probes
#
file(GLOB_RECURSE PROBE_FILES
     RELATIVE ${CMAKE_SOURCE_DIR}/share
     share/*.txt)
set(ALL_PROBE_FILES)
foreach(PROBE_FILE ${PROBE_FILES})
  set(src ${CMAKE_SOURCE_DIR}/share/${PROBE_FILE})
  set(tgt ${APPLICATION_RESOURCE_PATH}/Probes/${PROBE_FILE})
  add_custom_command(DEPENDS ${src}
                     COMMAND ${CMAKE_COMMAND} -E copy ${src} ${tgt}
                     OUTPUT ${tgt}
                     COMMENT "probe copy ${PROBE_FILE}")
  set(ALL_PROBE_FILES ${ALL_PROBE_FILES} ${tgt})
endforeach()

add_custom_target(CopyProbes ALL echo "..." DEPENDS ${ALL_PROBE_FILES})

install(FILES ${ALL_PROBE_FILES}
  DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources/Probes)

#
# Add Install Targets
#
if(EXISTS "${MAINFOLDER}/include/${PROJECT_NAME}" AND IS_DIRECTORY "${MAINFOLDER}/include/${PROJECT_NAME}")
    install(DIRECTORY "${MAINFOLDER}/include/${PROJECT_NAME}" DESTINATION "include")
endif(EXISTS "${MAINFOLDER}/include/${PROJECT_NAME}" AND IS_DIRECTORY "${MAINFOLDER}/include/${PROJECT_NAME}")
