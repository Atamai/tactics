PROJECT(ToolCursor)

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

SET(LIB_NAME vtk${PROJECT_NAME})
SET(LIB_VTK_LIBS vtkVolumeRendering vtkHybrid)
SET(LIB_SRCS
  vtkActionCursorShapes.cxx
  vtkFiducialPointsTool.cxx
  vtkFocalPlaneTool.cxx
  vtkFollowerPlane.cxx
  vtkGeometricCursorShapes.cxx
  vtkImageTool.cxx
  vtkImageToROIContourData.cxx
  vtkLassoImageTool.cxx
  vtkOpacityTool.cxx
  vtkPanCameraTool.cxx
  vtkPushPlaneTool.cxx
  vtkResliceMath.cxx
  vtkROIContourData.cxx
  vtkROIContourDataToPolyData.cxx
  vtkRotateCameraTool.cxx
  vtkSliceImageTool.cxx
  vtkSpinCameraTool.cxx
  vtkToolCursor.cxx
  vtkTool.cxx
  vtkToolCursorInteractorObserver.cxx
  vtkCursorShapes.cxx
  vtkSystemCursorShapes.cxx
  vtkWindowLevelTool.cxx
  vtkZoomCameraTool.cxx
)

SET(EXAMPLES
  LassoImageTool
)

# The remainder of this file can usually be left alone

FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})
IF(VTK_USE_CARBON)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch i386")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch i386")
ENDIF(VTK_USE_CARBON)

ADD_LIBRARY(${LIB_NAME} STATIC ${LIB_SRCS})
TARGET_LINK_LIBRARIES(${LIB_NAME} ${LIB_VTK_LIBS})

FOREACH(EXAMPLE ${EXAMPLES})
  ADD_EXECUTABLE(${EXAMPLE} ${EXAMPLE}.cxx)
  TARGET_LINK_LIBRARIES(${EXAMPLE} ${LIB_NAME})
ENDFOREACH(EXAMPLE ${EXAMPLES})

# Need to include class headers and the configuration header.
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR})

STRING(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)
STRING(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

# Build static, but use shared-library flags
IF(CMAKE_SHARED_LIBRARY_C_FLAGS)
SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES
                      COMPILE_FLAGS ${CMAKE_SHARED_LIBRARY_C_FLAGS})
ENDIF(CMAKE_SHARED_LIBRARY_C_FLAGS)

# Create the header to configure the classes.
SET(VTKLOCAL_BUILD_SHARED_LIBS_CONFIG 0)
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/vtkLocalConfigure.h.in
               ${PROJECT_BINARY_DIR}/${LIB_NAME}Configure.h
               @ONLY IMMEDIATE)

# Use wrapping hints for this project.
SET(VTK_WRAP_HINTS "${PROJECT_SOURCE_DIR}/hints")

# Create Python wrappers if VTK is wrapped in Python.
IF(VTK_WRAP_PYTHON)
  IF(VTK_PYTHON_INCLUDE_DIR)
    INCLUDE_DIRECTORIES("${VTK_PYTHON_INCLUDE_DIR}")
  ENDIF(VTK_PYTHON_INCLUDE_DIR)
  CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/vtkLocal.py.in
                 ${PROJECT_BINARY_DIR}/${PROJECT_NAME_LOWER}.py
                 @ONLY IMMEDIATE)
  INCLUDE(${VTK_CMAKE_DIR}/vtkWrapPython.cmake)
  VTK_WRAP_PYTHON3(${LIB_NAME}Python vtkLocalPYTHON_SRCS "${LIB_SRCS}")
  ADD_LIBRARY(${LIB_NAME}Python MODULE ${vtkLocalPYTHON_SRCS})
  FOREACH(c ${LIB_VTK_LIBS})
    TARGET_LINK_LIBRARIES(${LIB_NAME}Python ${c}PythonD)
  ENDFOREACH(c)
  TARGET_LINK_LIBRARIES(${LIB_NAME}Python ${LIB_NAME})
  SET_TARGET_PROPERTIES(${LIB_NAME}Python PROPERTIES PREFIX "")
  IF(WIN32)
    SET_TARGET_PROPERTIES(${LIB_NAME}Python PROPERTIES SUFFIX ".pyd")
  ENDIF(WIN32)
ENDIF(VTK_WRAP_PYTHON)
