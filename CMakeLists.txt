# Modernized CMakeLists.txt for VTK 9.6, Qt5/6, and clean CMake practices
cmake_minimum_required(VERSION 3.20)
project(Tactics VERSION 1.1.0 LANGUAGES CXX)

# ---------------------------------------------------------------
# Global Build Settings
# ---------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)

# ---------------------------------------------------------------
# Project Metadata
# ---------------------------------------------------------------
set(APPLICATION_NAME "Tactics")
set(APPLICATION_VENDOR_NAME "Calgary Image Processing and Analysis Centre")
set(APPLICATION_VENDOR_URL "calgaryimageanalysis.ca")

# ---------------------------------------------------------------
# Output Paths
# ---------------------------------------------------------------
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/lib")

file(MAKE_DIRECTORY
  "${EXECUTABLE_OUTPUT_PATH}"
  "${LIBRARY_OUTPUT_PATH}"
)

# ---------------------------------------------------------------
# Include Paths (Prefer target_include_directories later)
# ---------------------------------------------------------------
set(PROJECT_INCLUDE_DIRS
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/thirdparty/toolcursor"
  "${PROJECT_SOURCE_DIR}/src"
)

# ---------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------
# VTK
set(VTK_DIR "/Users/ystarrev/Development/vtkbin")
find_package(VTK 9.5 REQUIRED COMPONENTS
  CommonCore
  CommonDataModel
  FiltersCore
  FiltersSources
  IOImage
  InteractionStyle
  RenderingCore
  RenderingFreeType
  RenderingOpenGL2
)

# Qt (Prefer Qt6 if available)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# GDCM modern style
set(GDCM_DIR "/Users/ystarrev/Development/GDCMbin")
find_package(GDCM REQUIRED)

# ---------------------------------------------------------------
# Subdirectories
# ---------------------------------------------------------------
add_subdirectory(thirdparty)
add_subdirectory(src/gui)
add_subdirectory(src/data)
add_subdirectory(src/view)
add_subdirectory(src/dicom)
add_subdirectory(src/processing)
add_subdirectory(src/tactics)
add_subdirectory(test)

# ---------------------------------------------------------------
# Resources (Icons, Probes)
# ---------------------------------------------------------------
set(APPLICATION_ICON_PATH "${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.app/Contents/Icons")
set(APPLICATION_RESOURCE_PATH "${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.app/Contents/Resources")

foreach(ICONTYPE Icons Cursors)
  string(TOLOWER "${ICONTYPE}" LC_ICONTYPE)
  file(GLOB_RECURSE ICON_FILES "resources/${LC_ICONTYPE}/*.png")

  foreach(ICON_FILE ${ICON_FILES})
    set(dest "${APPLICATION_ICON_PATH}/${ICONTYPE}/${ICON_FILE}")
    add_custom_command(
      OUTPUT ${dest}
      DEPENDS ${ICON_FILE}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ICON_FILE} ${dest}
    )
    list(APPEND ALL_ICON_FILES ${dest})
  endforeach()

  add_custom_target(Copy${ICONTYPE} ALL DEPENDS ${ALL_ICON_FILES})
endforeach()

# Probes
file(GLOB_RECURSE PROBE_FILES "share/*.txt")
foreach(PROBE_FILE ${PROBE_FILES})
  set(dest "${APPLICATION_RESOURCE_PATH}/Probes/${PROBE_FILE}")
  add_custom_command(
    OUTPUT ${dest}
    DEPENDS ${PROBE_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROBE_FILE} ${dest}
  )
  list(APPEND ALL_PROBE_FILES ${dest})
endforeach()
add_custom_target(CopyProbes ALL DEPENDS ${ALL_PROBE_FILES})

# ---------------------------------------------------------------
# Install
# ---------------------------------------------------------------
install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}" DESTINATION include OPTIONAL)
