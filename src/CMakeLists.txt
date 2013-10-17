file(GLOB_RECURSE project_SRCS *.cpp *.cxx *.cc *.C *.c *.h *.hpp)
file(GLOB_RECURSE project_MAIN main.cpp main.cxx main.cc main.C main.c main.h main.hpp)
list(REMOVE_ITEM project_SRCS ${project_MAIN})
set(project_LIBS ${QT_LIBRARIES} ${VTK_LIBRARIES} ${ITK_LIBRARIES} ${Cerebra_LIBRARIES} cbVTK vtkImageRegistration vtkImageSegmentation)
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

qt4_wrap_cpp(project_MOC_SRCS_GENERATED ${project_MOC_HEADERS})

add_library(${project_LIB} STATIC ${project_SRCS} ${project_MOC_SRCS_GENERATED})
target_link_libraries(${project_LIB} ${project_LIBS})

add_executable(${project_BIN} MACOSX_BUNDLE ${project_MAIN} ${MACOSX_BUNDLE_ICON_FILE})
target_link_libraries(${project_BIN} ${project_LIB})
set_target_properties(${project_BIN} PROPERTIES VERSION "${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}" OUTPUT_NAME ${project_BIN} CLEAN_DIRECT_OUTPUT 1)

install(TARGETS ${project_BIN} DESTINATION bin)
