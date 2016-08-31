if(POLICY CMP0043)
  cmake_policy(SET CMP0043 OLD)
endif()

file(GLOB_RECURSE project_SRCS *.cpp *.cxx *.cc *.C *.c *.h *.hpp)
file(GLOB_RECURSE project_MAIN main.cpp main.cxx main.cc main.C main.c main.h main.hpp)
list(REMOVE_ITEM project_SRCS ${project_MAIN})
set(project_LIBS ${GDCM_LIBS} ${Cerebra_LIBRARIES} cbVTK vtkImageRegistration vtkImageSegmentation)
set(project_BIN ${PROJECT_NAME})
set(project_LIB cbElectrode)

set(project_MOC_HEADERS
  cbElectrodeAutomator.h
  cbElectrodeController.h
  cbElectrodeIntroductionStage.h
  cbElectrodeOpenStage.h
  cbElectrodePlanStage.h
  cbElectrodeToolBarWidget.h
  cbElectrodeView.h
)

add_library(${project_LIB} STATIC ${project_SRCS})
qt5_use_modules(${project_LIB} Widgets)
target_link_libraries(${project_LIB} ${project_LIBS})
set(project_BIN_SRCS ${project_MAIN})

if(APPLE)
  # set where in the bundle to put the icns file
  set_source_files_properties(${TACTICS_ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  set(project_BIN_SRCS ${project_BIN_SRCS} ${TACTICS_ICON_FILE})
endif()

add_executable(${project_BIN} MACOSX_BUNDLE ${project_BIN_SRCS})
target_link_libraries(${project_BIN} ${project_LIB} jsoncpp)
if(APPLE)
  set_target_properties(${project_BIN} PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH ON
    MACOSX_BUNDLE_BUNDLE_NAME Tactics
    MACOSX_BUNDLE_BUNDLE_VERSION "${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}"
    MACOSX_BUNDLE_ICON_FILE Tactics
    MACOSX_BUNDLE_GUI_IDENTIFIER ca.calgaryimageanalysis.Tactics
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}"
    MACOSX_BUNDLE_LONG_VERSION_STRING "${APPLICATION_VERSION_STRING}"
    MACOSX_BUNDLE_COPYRIGHT "Copyright 2016 David Gobbi and David Adair.")
else()
  set_target_properties(${project_BIN} PROPERTIES
    VERSION "${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}"
    OUTPUT_NAME ${project_BIN}
    CLEAN_DIRECT_OUTPUT 1)
endif()


#-------------------------------------------------------------------------------
# Use BundleUtilities to get all other dependencies for the application to work.
# It takes a bundle or executable along with possible plugins and inspects it
# for dependencies.  If they are not system dependencies, they are copied.

set(plugin_dest_dir bin)
set(qtconf_dest_dir bin)
set(APPS "\${CMAKE_INSTALL_PREFIX}/Tactics")
if(APPLE)
  set(plugin_dest_dir Tactics.app/Contents)
  set(qtconf_dest_dir Tactics.app/Contents/Resources)
  set(APPS "\${CMAKE_INSTALL_PREFIX}/Tactics.app")
endif()
if(WIN32)
  set(APPS "\${CMAKE_INSTALL_PREFIX}/bin/Tactics.exe")
endif()

# Install the project application, on Apple, the bundle is at the root of the
# install tree, and on other platforms it'll go into the bin directory.
install(TARGETS ${project_BIN}
    BUNDLE DESTINATION . COMPONENT Runtime
    RUNTIME DESTINATION bin COMPONENT Runtime
)

# Install needed Qt plugins by copying directories from the qt installation
foreach(plugin ${Qt5Gui_PLUGINS})
  get_target_property(src ${plugin} LOCATION)
  string(REGEX MATCH "plugins\\/*[^\\/]+\\/*[^\\/]+$" dst ${src})
  get_filename_component(dst ${dst} PATH)
  install(FILES ${src} DESTINATION ${plugin_dest_dir}/${dst}
          COMPONENT Runtime)
endforeach()

# install a qt.conf file
# this inserts some cmake code into the install script to write the file
install(CODE "
    file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${qtconf_dest_dir}/qt.conf\" \"\")
    " COMPONENT Runtime)

# directories to look for dependencies
set(DIRS
  ${VTK_DIR}
  ${CMAKE_INSTALL_PREFIX}/lib)

# Now the work of copying dependencies into the bundle/package
# Quotes and dollar signs must be escaped.
# An alternative is the do a configure_file() on a script and use
# install(SCRIPT  ...).
# The image plugins depend on QtSvg and QtXml
install(CODE "
file(GLOB_RECURSE QTPLUGINS
  \"\${CMAKE_INSTALL_PREFIX}/${plugin_dest_dir}/plugins/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
include(BundleUtilities)
fixup_bundle(\"${APPS}\" \"\${QTPLUGINS}\" \"${DIRS}\")
" COMPONENT Runtime)

# Bundle version information
set(CPACK_PACKAGE_VERSION_MAJOR ${APPLICATION_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${APPLICATION_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${APPLICATION_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${APPLICATION_VERSION_STRING})

# To bundle, run "cpack -G DragNDrop CPackConfig.cmake" on Mac OS X
# where CPackConfig.cmake is generated at configure time
set(CPACK_BINARY_DRAGNDROP ON)
include(CPack)
