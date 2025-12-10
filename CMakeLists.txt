cmake_minimum_required(VERSION 3.16)
project(TacticsProject LANGUAGES CXX)

# -------------------------------------------------------------------------
# Globals: versions / options
# -------------------------------------------------------------------------
set(APPLICATION_VERSION_MAJOR 1)
set(APPLICATION_VERSION_MINOR 0)
set(APPLICATION_VERSION_PATCH 0)
set(APPLICATION_VERSION_STRING "${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}.${APPLICATION_VERSION_PATCH}")

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------------------------
# Make a variable many small CMakeLists in the tree rely on
# -------------------------------------------------------------------------
# MAINFOLDER used by many child CMakeLists in this repo
set(MAINFOLDER "${CMAKE_SOURCE_DIR}" CACHE PATH "Project root (used by subprojects)")

# A path used by older child CMakeLists to append exported targets
set(CONFIGURE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/cmake" CACHE PATH "Where generated target files are written")

# Convenience: install prefix default (macOS-friendly)
if(NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Install path" FORCE)
endif()

# -------------------------------------------------------------------------
# Enable Qt automagic for all subprojects
# -------------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt once at top level so child CMakeLists can rely on Qt5:: targets
find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui Svg Xml)

# -------------------------------------------------------------------------
# Find VTK once at top level (modules may vary — children can find more if needed)
# -------------------------------------------------------------------------
find_package(VTK 9.5 REQUIRED COMPONENTS
  CommonCore
  CommonDataModel
  FiltersCore
  FiltersGeneral
  IOImage
)

# make VTK targets available to subdirectories
if(VTK_VERSION)
  message(STATUS "Found VTK ${VTK_VERSION}")
  # export VTK include dirs / targets already done by find_package
endif()

# -------------------------------------------------------------------------
# Useful convenience variables for children
# -------------------------------------------------------------------------
# Where third-party code lives
set(THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/thirdparty" CACHE PATH "Thirdparty directory")

# make vars available to children
set(MAINFOLDER ${MAINFOLDER} PARENT_SCOPE)
set(CONFIGURE_OUTPUT_PATH ${CONFIGURE_OUTPUT_PATH} PARENT_SCOPE)

# -------------------------------------------------------------------------
# Add thirdparty before project modules if present (toolcursor etc)
# -------------------------------------------------------------------------
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/CMakeLists.txt")
  add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty")
endif()

# -------------------------------------------------------------------------
# Add project modules in dependency order
# (data -> view -> gui -> processing -> tactics app)
# -------------------------------------------------------------------------
# cbData
if(EXISTS "${CMAKE_SOURCE_DIR}/src/data/CMakeLists.txt")
  add_subdirectory("${CMAKE_SOURCE_DIR}/src/data")
endif()

# cbVTK or other view-level modules
if(EXISTS "${CMAKE_SOURCE_DIR}/src/view/CMakeLists.txt")
  add_subdirectory("${CMAKE_SOURCE_DIR}/src/view")
endif()

# cbGUI
if(EXISTS "${CMAKE_SOURCE_DIR}/src/gui/CMakeLists.txt")
  add_subdirectory("${CMAKE_SOURCE_DIR}/src/gui")
endif()

# processing (utilities)
if(EXISTS "${CMAKE_SOURCE_DIR}/src/processing/CMakeLists.txt")
  add_subdirectory("${CMAKE_SOURCE_DIR}/src/processing")
endif()

# tactics app (depends on the libs above)
if(EXISTS "${CMAKE_SOURCE_DIR}/src/tactics/CMakeLists.txt")
  add_subdirectory("${CMAKE_SOURCE_DIR}/src/tactics")
endif()

# -------------------------------------------------------------------------
# Provide a convenient 'all-libs' target that depends on exported static libs
# so 'cmake --build . --target all-libs' works. We only add if the libs exist.
# -------------------------------------------------------------------------
add_custom_target(all-libs)
if(TARGET cbData)
  add_dependencies(all-libs cbData)
endif()
if(TARGET cbVTK)
  add_dependencies(all-libs cbVTK)
endif()
if(TARGET cbView)
  add_dependencies(all-libs cbView)
endif()
if(TARGET cbGUI)
  add_dependencies(all-libs cbGUI)
endif()
if(TARGET cbElectrode)
  add_dependencies(all-libs cbElectrode)
endif()

# -------------------------------------------------------------------------
# Install and packaging helpers (minimal)
# -------------------------------------------------------------------------
include(GNUInstallDirs)
# ensure configure output directory exists
file(MAKE_DIRECTORY "${CONFIGURE_OUTPUT_PATH}")

# Print summary
message(STATUS "Project root (MAINFOLDER): ${MAINFOLDER}")
message(STATUS "Configure output path: ${CONFIGURE_OUTPUT_PATH}")
message(STATUS "CMake install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Qt5 found: ${Qt5_VERSION}")
message(STATUS "VTK found: ${VTK_VERSION}")

# End of top-level CMake
