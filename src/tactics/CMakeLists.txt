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
#  Qt Setup (Modern)
# ==============================================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Widgets Gui Core Svg Xml)

# ==============================================================================
#  VTK Setup (Modern Target-based)
# ==============================================================================
find_package(VTK 9.5 REQUIRED
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

# Include paths for your library (PUBLIC so executable inherits)
target_include_directories(${PROJECT_LIB}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui
        ${CMAKE_CURRENT_SOURCE_DIR}/../view
        ${CMAKE_CURRENT_SOURCE_DIR}/../data
)

# Link libraries (Qt, VTK, internal)
target_link_libraries(${PROJECT_LIB}
    PRIVATE
        Qt5::Core
        Qt5::Widgets
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

# Link executable against library and dependencies
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_LIB}
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
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
#  macOS bundle variables
# ==============================================================================
if(APPLE)
    set(plugin_dest_dir "${PROJECT_NAME}.app/Contents")
    set(qtconf_dest_dir "${PROJECT_NAME}.app/Contents/Resources")
    set(APPS "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app")
else()
    set(plugin_dest_dir "bin")
    set(qtconf_dest_dir "bin")
    set(APPS "${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME}")
endif()

# ==============================================================================
#  macOS bundle properties
# ==============================================================================
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH_USE_LINK_PATH ON
        MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
        MACOSX_BUNDLE_ICON_FILE Tactics
        MACOSX_BUNDLE_GUI_IDENTIFIER ca.calgaryimageanalysis.${PROJECT_NAME}
    )
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_NAME ${PROJECT_NAME}
        VERSION "${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}"
        CLEAN_DIRECT_OUTPUT 1
    )
endif()

# ==============================================================================
#  Installation
# ==============================================================================
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .        COMPONENT Runtime
    RUNTIME DESTINATION bin     COMPONENT Runtime
)

# qt.conf for relocatability
install(CODE "
    file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${qtconf_dest_dir}/qt.conf\" \"\")
" COMPONENT Runtime)

# ==============================================================================
#  macOS fixup_bundle (in
