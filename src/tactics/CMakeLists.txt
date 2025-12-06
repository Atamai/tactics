cmake_minimum_required(VERSION 3.16)
project(Tactics LANGUAGES CXX)

# ==============================================================================
#  POLICIES
# ==============================================================================
if(POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW)
endif()

# ==============================================================================
#  QT SETUP (Modern)
# ==============================================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Gui Core Svg Xml)

# ==============================================================================
#  SOURCE FILE COLLECTION
# ==============================================================================
file(GLOB_RECURSE project_SRCS
    CONFIGURE_DEPENDS
    *.cpp *.cxx *.cc *.C
    *.h *.hpp)

file(GLOB_RECURSE project_MAIN
    CONFIGURE_DEPENDS
    main.cpp main.cxx main.cc main.C
    main.h main.hpp)

list(REMOVE_ITEM project_SRCS ${project_MAIN})

set(project_LIB cbElectrode)
set(project_BIN ${PROJECT_NAME})

set(project_LIBS
    ${GDCM_LIBS}
    ${Cerebra_LIBRARIES}
    cbVTK
    vtkImageRegistration
    vtkImageSegmentation
)

# ==============================================================================
#  DESTINATION / BUNDLE PATHS (Define BEFORE install() commands!)
# ==============================================================================
# Defaults (Linux)
set(plugin_dest_dir bin)
set(qtconf_dest_dir bin)
set(APPS "\${CMAKE_INSTALL_PREFIX}/Tactics")

if(APPLE)
    set(plugin_dest_dir Tactics.app/Contents)
    set(qtconf_dest_dir Tactics.app/Contents/Resources)
    set(APPS "\${CMAKE_INSTALL_PREFIX}/Tactics.app")
elseif(WIN32)
    set(APPS "\${CMAKE_INSTALL_PREFIX}/bin/Tactics.exe")
endif()

# ==============================================================================
#  LIBRARY: cbElectrode
# ==============================================================================
add_library(${project_LIB} STATIC ${project_SRCS})

target_link_libraries(${project_LIB}
    PRIVATE
        ${project_LIBS}
        Qt5::Core
        Qt5::Gui
        Qt5::Svg
        Qt5::Xml
        Qt5::Widgets
)

# ==============================================================================
#  EXECUTABLE: Tactics
# ==============================================================================
set(project_BIN_SRCS ${project_MAIN})

if(APPLE)
    set_source_files_properties(${TACTICS_ICON_FILE}
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    list(APPEND project_BIN_SRCS ${TACTICS_ICON_FILE})
endif()

add_executable(${project_BIN} MACOSX_BUNDLE ${project_BIN_SRCS})

target_link_libraries(${project_BIN}
    PRIVATE
        ${project_LIB}
        jsoncpp
)

# ------------------------------------------------------------------------------
# macOS bundle metadata
# ------------------------------------------------------------------------------
if(APPLE)
    set_target_properties(${project_BIN} PROPERTIES
        INSTALL_RPATH_USE_LINK_PATH ON
        MACOSX_BUNDLE_BUNDLE_NAME Tactics
        MACOSX_BUNDLE_BUNDLE_VERSION "${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}"
        MACOSX_BUNDLE_ICON_FILE Tactics
        MACOSX_BUNDLE_GUI_IDENTIFIER ca.calgaryimageanalysis.Tactics
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${APPLICATION_VERSION_STRING}"
        MACOSX_BUNDLE_COPYRIGHT "Copyright 2016 David Gobbi and David Adair."
    )
else()
    set_target_properties(${project_BIN} PROPERTIES
        VERSION "${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}"
        OUTPUT_NAME ${project_BIN}
        CLEAN_DIRECT_OUTPUT 1
    )
endif()

# ==============================================================================
#  INSTALLATION
# ==============================================================================
install(TARGETS ${project_BIN}
    BUNDLE DESTINATION .        COMPONENT Runtime
    RUNTIME DESTINATION bin     COMPONENT Runtime
)

# ==============================================================================
#  QT PLUGIN INSTALLATION
# ==============================================================================
foreach(plugin ${Qt5Gui_PLUGINS})
    install(FILES "$<TARGET_FILE:${plugin}>"
            DESTINATION ${plugin_dest_dir}
            COMPONENT Runtime)
endforeach()

# ==============================================================================
#  qt.conf (required for macOS bundle relocatability)
# ==============================================================================
install(CODE "
    file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${qtconf_dest_dir}/qt.conf\" \"\")
" COMPONENT Runtime)

# ==============================================================================
#  BundleUtilities fixup_bundle
# ==============================================================================
set(DIRS
    ${VTK_DIR}
    ${CMAKE_INSTALL_PREFIX}/lib
)

install(CODE "
file(GLOB_RECURSE QTPLUGINS
  \"\${CMAKE_INSTALL_PREFIX}/${plugin_dest_dir}/plugins/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
include(BundleUtilities)
fixup_bundle(\"${APPS}\" \"\${QTPLUGINS}\" \"${DIRS}\")
" COMPONENT Runtime)

# ==============================================================================
#  CPack PACKAGE SETTINGS
# ==============================================================================
set(CPACK_PACKAGE_VERSION_MAJOR ${APPLICATION_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${APPLICATION_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${APPLICATION_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION       ${APPLICATION_VERSION_STRING})

set(CPACK_BINARY_DRAGNDROP ON)  # macOS .dmg

include(CPack)
