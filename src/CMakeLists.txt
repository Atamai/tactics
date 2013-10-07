file(GLOB_RECURSE project_SRCS *.cpp *.cxx *.cc *.C *.c *.h *.hpp)
set(project_LIBS ${Boost_LIBRARIES} ${QT_LIBRARIES} ${VTK_LIBRARIES} ${ITK_LIBRARIES} ${Cerebra_LIBRARIES})
set(project_BIN ${PROJECT_NAME})

message(STATUS "Cerebra-libs: ${Cerebra_LIBRARIES}")

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

add_executable(${project_BIN} ${project_SRCS} ${project_MOC_SRCS_GENERATED})
target_link_libraries(${project_BIN} ${project_LIBS} cbVTK vtkImageSegmentation vtkImageRegistration)
set_target_properties(${project_BIN} PROPERTIES VERSION "${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}" OUTPUT_NAME ${project_BIN} CLEAN_DIRECT_OUTPUT 1)

install(TARGETS ${project_BIN} DESTINATION bin)
