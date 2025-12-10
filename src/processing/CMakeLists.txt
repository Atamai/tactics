cmake_minimum_required(VERSION 3.16)
project(cbProcessing LANGUAGES CXX)

# ------------------------------------------------------------------------
# Policies
# ------------------------------------------------------------------------
if(POLICY CMP0003)
  cmake_policy(SET CMP0003 NEW)
endif()

# ------------------------------------------------------------------------
# Export to parent scope (unchanged)
# ------------------------------------------------------------------------
set(Cerebra_LIBRARIES ${Cerebra_LIBRARIES} ${PROJECT_NAME} PARENT_SCOPE)
set(INCLUDE_OUTPUT_PATH ${INCLUDE_OUTPUT_PATH} ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

# ------------------------------------------------------------------------
# Find VTK (modern)
# ------------------------------------------------------------------------
find_package(VTK 9.5 REQUIRED
  COMPONENTS
    CommonCore
    CommonDataModel
    ImagingCore
    ImagingMath
    ImagingStatistics
)

find_package(AIRS REQUIRED)


# ------------------------------------------------------------------------
# Create library
# ------------------------------------------------------------------------
add_library(${PROJECT_NAME} STATIC ${LIB_SRCS})

# ------------------------------------------------------------------------
# Include dirs
# ------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

# ------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------
set(LIB_SRCS
  cbMRIRegistration.cxx
)



# ------------------------------------------------------------------------
# Link modern VTK targets
# ------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
  VTK::CommonCore
  VTK::CommonDataModel
  VTK::ImagingCore
  VTK::ImagingMath
  VTK::ImagingStatistics
  VTK::ImageRegistration
  VTK::ImageSegmentation
  cbVTK
)

# ------------------------------------------------------------------------
# VTK module autoinit
# ------------------------------------------------------------------------
vtk_module_autoinit(
  TARGETS ${PROJECT_NAME}
  MODULES
    VTK::CommonCore
    VTK::CommonDataModel
    VTK::ImagingCore
    VTK::ImagingMath
    VTK::ImagingStatistics
)

# ------------------------------------------------------------------------
# Export target
# ------------------------------------------------------------------------
export(
  TARGETS ${PROJECT_NAME}
  APPEND FILE "${CONFIGURE_OUTPUT_PATH}/CerebraTargets.cmake"
)
