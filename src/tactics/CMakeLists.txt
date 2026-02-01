cmake_minimum_required(VERSION 3.16)
project(Tactics LANGUAGES CXX)

# ==============================================================================
#  Project version
# ==============================================================================
set(APPLICATION_VERSION_MAJOR 1)
set(APPLICATION_VERSION_MINOR 0)
set(APPLICATION_VERSION_PATCH 0)
set(APPLICATION_VERSION_STRING "${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}.${APPLICATION_VERSION_PATCH}")

# ==============================================================================
#  Qt Setup
# ==============================================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core Svg Xml)

# ==============================================================================
#  VTK Setup
# ==============================================================================
find_package(VTK 9.6 REQUIRED
    COMPONENTS
        CommonCore
        CommonDataModel
        FiltersCore
        RenderingFreeType
        RenderingVolumeOpenGL2
        RenderingAnnotation
        FiltersGeneral
        InteractionStyle
        IOImage
        jsoncpp
)

find_package(ToolCursor REQUIRED)
find_package(AIRS REQUIRED)
find_package(DICOM REQUIRED)

# ==============================================================================
#  Project source files
# ==============================================================================
file(GLOB_RECURSE PROJECT_SRCS CONFIGURE_DEPENDS *.cxx)
file(GLOB_RECURSE PROJECT_HDRS CONFIGURE_DEPENDS *.h)
file(GLOB_RECURSE PROJECT_MAIN CONFIGURE_DEPENDS main.cxx)
list(REMOVE_ITEM PROJECT_SRCS ${PROJECT_MAIN})

# ==============================================================================
#  Library: cbElectrode
# ==============================================================================
set(PROJECT_LIB cbElectrode)
add_library(${PROJECT_LIB} STATIC ${PROJECT_SRCS} ${PROJECT_HDRS})

target_include_directories(${PROJECT_LIB}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui
        ${CMAKE_CURRENT_SOURCE_DIR}/../view
        ${CMAKE_CURRENT_SOURCE_DIR}/../data
)

target_link_libraries(${PROJECT_LIB}
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        ${Cerebra_LIBRARIES}
        cbVTK
        cbProcessing
        VTK::CommonCore
        VTK::RenderingFreeType
        VTK::RenderingAnnotation
        VTK::RenderingVolumeOpenGL2
        VTK::InteractionStyle
        VTK::DICOM
        VTK::jsoncpp
)

# ==============================================================================
#  Executable: Tactics
# ==============================================================================
add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${PROJECT_MAIN})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_LIB}
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        VTK::CommonCore
)

# ==============================================================================
#  VTK automatic module initialization
# ==============================================================================
vtk_module_autoinit(
    TARGETS ${PROJECT_LIB}
    MODULES
        VTK::CommonCore
        VTK::CommonDataModel
        VTK::FiltersCore
        VTK::RenderingVolumeOpenGL2
        VTK::FiltersSources
        VTK::FiltersGeneral
        VTK::IOImage
)

# ==============================================================================
#  macOS bundle properties
# ==============================================================================
if(APPLE)
    set(APP_ICON
        ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/Tactics.icns
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
        MACOSX_BUNDLE_ICON_FILE Tactics
        MACOSX_BUNDLE_GUI_IDENTIFIER ca.calgaryimageanalysis.${PROJECT_NAME}
    )
    set_source_files_properties(${APP_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
    target_sources(${PROJECT_NAME} PRIVATE ${APP_ICON})
endif()

# ==============================================================================
#  Resource directories
# ==============================================================================
set(ICONS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/icons")
set(CURSORS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/cursors")
set(PROBES_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../share")

if(APPLE)
    set(BUNDLE_DIR "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.app/Contents")
    set(ICONS_DST_DIR "${BUNDLE_DIR}/Icons")
    set(CURSORS_DST_DIR "${BUNDLE_DIR}/Cursors")
    set(PROBES_DST_DIR "${BUNDLE_DIR}/Resources/Probes")
else()
    set(ICONS_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/Resources/Icons")
    set(CURSORS_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/Resources/Cursors")
    set(PROBES_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/Resources/Probes")
endif()

# ==============================================================================
#  Copy icons
# ==============================================================================
if(EXISTS "${ICONS_SRC_DIR}")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ICONS_DST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${ICONS_SRC_DIR}" "${ICONS_DST_DIR}"
    )
endif()

# ==============================================================================
#  Copy cursors
# ==============================================================================
if(EXISTS "${CURSORS_SRC_DIR}")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CURSORS_DST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CURSORS_SRC_DIR}" "${CURSORS_DST_DIR}"
    )
endif()

# ==============================================================================
#  Copy probe files individually
# ==============================================================================
file(GLOB PROBE_FILES "${PROBES_SRC_DIR}/*.txt")
if(PROBE_FILES)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROBES_DST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy ${PROBE_FILES} "${PROBES_DST_DIR}"
    )
endif()

# ==============================================================================
#  Installation configuration for portable bundle
# ==============================================================================
if(APPLE)
    # Install the application bundle
    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION . COMPONENT Runtime
        RUNTIME DESTINATION bin COMPONENT Runtime
    )

    # Get Qt6 installation path - try multiple methods
    get_target_property(_qt6_qmake_location Qt6::qmake IMPORTED_LOCATION)
    if(_qt6_qmake_location)
        get_filename_component(_qt6_install_prefix "${_qt6_qmake_location}/../../" ABSOLUTE)
    else()
        # Fallback: use Qt6_DIR
        get_filename_component(_qt6_install_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
    endif()
    
    message(STATUS "Qt6 install prefix: ${_qt6_install_prefix}")
    message(STATUS "Qt6_DIR: ${Qt6_DIR}")
    if(_qt6_qmake_location)
        message(STATUS "qmake location: ${_qt6_qmake_location}")
    endif()
    
    # Find Qt6 plugins directory
    # For Homebrew installations, try the most common locations first
    set(_qt6_plugins_dir "")
    
    if(EXISTS "/opt/homebrew/share/qt/plugins/platforms")
        set(_qt6_plugins_dir "/opt/homebrew/share/qt/plugins")
    elseif(EXISTS "/opt/homebrew/Cellar/qtbase/6.10.1/share/qt/plugins/platforms")
        set(_qt6_plugins_dir "/opt/homebrew/Cellar/qtbase/6.10.1/share/qt/plugins")
    else()
        # Try to find based on Qt6_DIR
        set(_possible_plugin_dirs
            "${Qt6_DIR}/../../../share/qt/plugins"
            "${Qt6_DIR}/../../../plugins"
            "${_qt6_install_prefix}/share/qt/plugins"
            "${_qt6_install_prefix}/plugins"
        )
        
        foreach(_plugin_path ${_possible_plugin_dirs})
            get_filename_component(_plugin_path_abs "${_plugin_path}" ABSOLUTE)
            if(EXISTS "${_plugin_path_abs}/platforms")
                set(_qt6_plugins_dir "${_plugin_path_abs}")
                break()
            endif()
        endforeach()
    endif()
    
    if(_qt6_plugins_dir)
        message(STATUS "Qt6 plugins directory: ${_qt6_plugins_dir}")
    else()
        message(FATAL_ERROR "Could not find Qt6 plugins directory. Please install Qt6 via Homebrew or set QT6_PLUGINS_DIR manually.")
    endif()

    # Install Qt plugins using file(INSTALL) at install time
    install(CODE "
        set(_qt_plugins_source \"${_qt6_plugins_dir}\")
        set(_qt_plugins_dest \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/PlugIns\")
        
        message(STATUS \"========================================\")
        message(STATUS \"Installing Qt plugins\")
        message(STATUS \"  Source: \${_qt_plugins_source}\")
        message(STATUS \"  Dest:   \${_qt_plugins_dest}\")
        message(STATUS \"========================================\")
        
        # Verify source exists
        if(NOT EXISTS \"\${_qt_plugins_source}\")
            message(FATAL_ERROR \"Qt plugins source directory does not exist: \${_qt_plugins_source}\")
        endif()
        
        # Create destination directory
        file(MAKE_DIRECTORY \"\${_qt_plugins_dest}\")
        
        # Copy platforms (REQUIRED)
        if(EXISTS \"\${_qt_plugins_source}/platforms\")
            file(GLOB _platform_plugins \"\${_qt_plugins_source}/platforms/*.dylib\")
            list(FILTER _platform_plugins EXCLUDE REGEX \".*_debug\\\\.dylib$\")
            foreach(_plugin \${_platform_plugins})
                get_filename_component(_plugin_name \${_plugin} NAME)
                file(INSTALL \${_plugin} DESTINATION \"\${_qt_plugins_dest}/platforms\")
                message(STATUS \"  Installed: platforms/\${_plugin_name}\")
            endforeach()
        else()
            message(FATAL_ERROR \"Qt platforms plugins not found at: \${_qt_plugins_source}/platforms\")
        endif()
        
        # Copy styles (optional but recommended)
        if(EXISTS \"\${_qt_plugins_source}/styles\")
            file(GLOB _style_plugins \"\${_qt_plugins_source}/styles/*.dylib\")
            list(FILTER _style_plugins EXCLUDE REGEX \".*_debug\\\\.dylib$\")
            foreach(_plugin \${_style_plugins})
                get_filename_component(_plugin_name \${_plugin} NAME)
                file(INSTALL \${_plugin} DESTINATION \"\${_qt_plugins_dest}/styles\")
                message(STATUS \"  Installed: styles/\${_plugin_name}\")
            endforeach()
        endif()
        
        # Copy imageformats (optional)
        if(EXISTS \"\${_qt_plugins_source}/imageformats\")
            file(GLOB _image_plugins \"\${_qt_plugins_source}/imageformats/*.dylib\")
            list(FILTER _image_plugins EXCLUDE REGEX \".*_debug\\\\.dylib$\")
            foreach(_plugin \${_image_plugins})
                get_filename_component(_plugin_name \${_plugin} NAME)
                file(INSTALL \${_plugin} DESTINATION \"\${_qt_plugins_dest}/imageformats\")
                message(STATUS \"  Installed: imageformats/\${_plugin_name}\")
            endforeach()
        endif()
        
        message(STATUS \"========================================\")
        message(STATUS \"Qt plugins installation complete\")
        message(STATUS \"========================================\")
    " COMPONENT Runtime)

    # Install qt.conf for plugin discovery
    install(CODE "
        file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources/qt.conf\"
            \"[Paths]\\nPlugins = PlugIns\\n\")
    " COMPONENT Runtime)

    # Collect library search paths for dependencies
    set(BUNDLE_LIBS_SEARCH_DIRS
        "${_qt6_install_prefix}/lib"
        "${VTK_DIR}/../.."
        "${VTK_INSTALL_PREFIX}/lib"
        "${VTK_DIR}/../../lib"
    )

    # Add paths for custom libraries if they have known locations
    if(DEFINED ToolCursor_DIR)
        list(APPEND BUNDLE_LIBS_SEARCH_DIRS "${ToolCursor_DIR}/../lib")
    endif()
    if(DEFINED AIRS_DIR)
        list(APPEND BUNDLE_LIBS_SEARCH_DIRS "${AIRS_DIR}/../lib")
    endif()
    if(DEFINED DICOM_DIR)
        list(APPEND BUNDLE_LIBS_SEARCH_DIRS "${DICOM_DIR}/../lib")
    endif()

    # Use Qt's macdeployqt if available, otherwise use fixup_bundle
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS ${_qt6_install_prefix}/bin)
    
    if(MACDEPLOYQT_EXECUTABLE)
        message(STATUS "Found macdeployqt: ${MACDEPLOYQT_EXECUTABLE}")
        
        # Use macdeployqt for Qt deployment, then fixup for other libraries
        install(CODE "
            message(STATUS \"Running macdeployqt...\")
            execute_process(
                COMMAND \"${MACDEPLOYQT_EXECUTABLE}\" \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app\" -verbose=2 -always-overwrite
                RESULT_VARIABLE _macdeployqt_result
                OUTPUT_VARIABLE _macdeployqt_output
                ERROR_VARIABLE _macdeployqt_error
            )
            
            if(NOT _macdeployqt_result EQUAL 0)
                message(WARNING \"macdeployqt failed: \${_macdeployqt_error}\")
            else()
                message(STATUS \"macdeployqt completed successfully\")
            endif()
            
            # Now use fixup_bundle for non-Qt libraries (VTK, custom libs)
            include(BundleUtilities)
            set(BU_CHMOD_BUNDLE_ITEMS ON)
            
            # Library search directories for VTK and custom libraries
            set(DIRS ${BUNDLE_LIBS_SEARCH_DIRS})
            
            # Fix up remaining dependencies
            fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app\" \"\" \"\${DIRS}\")
            
            # Proper code signing after all fixups (order matters)
            message(STATUS \"Re-signing bundle components...\")
            
            # Remove existing signatures first
            file(GLOB_RECURSE _all_dylibs \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Frameworks/*.dylib\")
            foreach(_lib \${_all_dylibs})
                execute_process(COMMAND codesign --remove-signature \"\${_lib}\" OUTPUT_QUIET ERROR_QUIET)
            endforeach()
            
            file(GLOB_RECURSE _all_plugins \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/PlugIns/*.dylib\")
            foreach(_plugin \${_all_plugins})
                execute_process(COMMAND codesign --remove-signature \"\${_plugin}\" OUTPUT_QUIET ERROR_QUIET)
            endforeach()
            
            execute_process(COMMAND codesign --remove-signature \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/MacOS/${PROJECT_NAME}\" OUTPUT_QUIET ERROR_QUIET)
            
            # Re-sign in proper order: libraries -> plugins -> frameworks -> executable -> bundle
            # 1. Sign all .dylib files in Frameworks
            foreach(_lib \${_all_dylibs})
                execute_process(COMMAND codesign --force --sign - \"\${_lib}\" OUTPUT_QUIET ERROR_QUIET)
            endforeach()
            
            # 2. Sign all Qt plugins
            foreach(_plugin \${_all_plugins})
                execute_process(COMMAND codesign --force --sign - \"\${_plugin}\" OUTPUT_QUIET ERROR_QUIET)
            endforeach()
            
            # 3. Sign all frameworks
            file(GLOB _frameworks \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Frameworks/*.framework\")
            foreach(_framework \${_frameworks})
                execute_process(COMMAND codesign --force --sign - \"\${_framework}\" OUTPUT_QUIET ERROR_QUIET)
            endforeach()
            
            # 4. Sign the main executable
            execute_process(COMMAND codesign --force --sign - \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/MacOS/${PROJECT_NAME}\" OUTPUT_QUIET ERROR_QUIET)
            
            # 5. Sign the entire bundle
            execute_process(
                COMMAND codesign --force --sign - \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app\"
                RESULT_VARIABLE _codesign_result
            )
            
            if(_codesign_result EQUAL 0)
                message(STATUS \"Bundle signed successfully\")
            else()
                message(WARNING \"Bundle signing failed\")
            endif()
            
            # Verify the bundle
            verify_app(\"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app\")
        " COMPONENT Runtime)
    else()
        message(STATUS "macdeployqt not found, using fixup_bundle only")
        
        # Use fixup_bundle to make the app portable
        install(CODE "
            include(BundleUtilities)
            set(BU_CHMOD_BUNDLE_ITEMS ON)
            set(APPS \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app\")
            
            # Collect Qt plugins - only those that exist
            set(QTPLUGINS \"\")
            if(EXISTS \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/PlugIns\")
                file(GLOB_RECURSE _all_plugins
                    \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/PlugIns/*.dylib\")
                foreach(_plugin \${_all_plugins})
                    if(EXISTS \"\${_plugin}\")
                        list(APPEND QTPLUGINS \"\${_plugin}\")
                    endif()
                endforeach()
            endif()
            
            list(LENGTH QTPLUGINS _plugin_count)
            message(STATUS \"Processing \${_plugin_count} Qt plugins\")
            
            # Library search directories
            set(DIRS ${BUNDLE_LIBS_SEARCH_DIRS})
            
            # Fix up the bundle - this copies all dependent libraries and fixes their paths
            fixup_bundle(\"\${APPS}\" \"\${QTPLUGINS}\" \"\${DIRS}\")
            
            # Re-sign the bundle properly (order matters: libraries -> frameworks -> plugins -> executable -> bundle)
            message(STATUS \"Re-signing bundle components...\")
            
            # 1. Sign all .dylib files in Frameworks
            file(GLOB_RECURSE _framework_libs \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Frameworks/*.dylib\")
            foreach(_lib \${_framework_libs})
                execute_process(
                    COMMAND codesign --force --sign - \"\${_lib}\"
                    OUTPUT_QUIET ERROR_QUIET
                )
            endforeach()
            
            # 2. Sign all Qt plugins
            file(GLOB_RECURSE _plugin_libs \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/PlugIns/*.dylib\")
            foreach(_plugin \${_plugin_libs})
                execute_process(
                    COMMAND codesign --force --sign - \"\${_plugin}\"
                    OUTPUT_QUIET ERROR_QUIET
                )
            endforeach()
            
            # 3. Sign all frameworks
            file(GLOB _frameworks \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Frameworks/*.framework\")
            foreach(_framework \${_frameworks})
                execute_process(
                    COMMAND codesign --force --sign - \"\${_framework}\"
                    OUTPUT_QUIET ERROR_QUIET
                )
            endforeach()
            
            # 4. Sign the main executable
            execute_process(
                COMMAND codesign --force --sign - \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/MacOS/${PROJECT_NAME}\"
                OUTPUT_QUIET ERROR_QUIET
            )
            
            # 5. Sign the entire bundle (without --deep to avoid conflicts)
            execute_process(
                COMMAND codesign --force --sign - \"\${APPS}\"
                RESULT_VARIABLE _codesign_result
                OUTPUT_VARIABLE _codesign_output
                ERROR_VARIABLE _codesign_error
            )
            
            if(NOT _codesign_result EQUAL 0)
                message(WARNING \"Bundle signing failed: \${_codesign_error}\")
            else()
                message(STATUS \"Bundle signed successfully\")
            endif()
            
            # Verify the bundle is portable
            verify_app(\"\${APPS}\")
        " COMPONENT Runtime)
    endif()

    # Create a deployable DMG
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME}")
    set(CPACK_DMG_FORMAT "UDBZ")
    set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/Tactics.icns")
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VERSION "${APPLICATION_VERSION_STRING}")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${APPLICATION_VERSION_STRING}")
    set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE OFF)
    
    # This creates a nice DMG with just the app bundle
    set(CPACK_PACKAGE_EXECUTABLES "${PROJECT_NAME}" "${PROJECT_NAME}")
    
    include(CPack)

endif()
