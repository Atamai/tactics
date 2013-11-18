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

#
# Project Data Path
#
set(APPLICATION_DATA_PATH "${MAINFOLDER}/share")
set(APPLICATION_ICON_PATH
  "${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.app/Contents/Icons")

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
# Application icon
#
set(TACTICS_ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/resources/Tactics.icns)

#
# Add Build Targets
#
add_subdirectory(src)
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
# Add Install Targets
#
if(EXISTS "${MAINFOLDER}/include/${PROJECT_NAME}" AND IS_DIRECTORY "${MAINFOLDER}/include/${PROJECT_NAME}")
    install(DIRECTORY "${MAINFOLDER}/include/${PROJECT_NAME}" DESTINATION "include")
endif(EXISTS "${MAINFOLDER}/include/${PROJECT_NAME}" AND IS_DIRECTORY "${MAINFOLDER}/include/${PROJECT_NAME}")

#
# Add Documentation Targets
#
include(DocumentationTargets)
