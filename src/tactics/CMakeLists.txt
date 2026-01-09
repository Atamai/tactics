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
#  qt.conf for relocatability
# ==============================================================================
if(APPLE)
    set(QTCONF_DIR "${BUNDLE_DIR}")
else()
    set(QTCONF_DIR "${CMAKE_CURRENT_BINARY_DIR}")
endif()

install(CODE "
file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${QTCONF_DIR}/qt.conf\"
\"[Paths]\nPlugins = PlugIns\n\")
" COMPONENT Runtime)
