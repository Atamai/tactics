include_directories(
  "${MAINFOLDER}/src/data"
  "${MAINFOLDER}/src/dicom"
  "${MAINFOLDER}/src/view"
  "${MAINFOLDER}/src/gui"
  "${MAINFOLDER}/src/processing"
  )

file(GLOB_RECURSE test_SRCS *.cpp *.cxx *.cc *.C *.c *.h *.hpp)
set(test_LIBS ${Qt_LIBRARIES} ${UnitTestPlusPlus_LIBRARIES} ${Cerebra_LIBRARIES} vtkHybrid vtkCommon)
set(test_BIN ${PROJECT_NAME}-unittests)

add_executable(${test_BIN} ${test_SRCS})
target_link_libraries(${test_BIN} ${test_LIBS})

add_custom_target(check ALL "${MAINFOLDER}/bin/${test_BIN}" DEPENDS ${test_BIN} COMMENT "Executing unit tests..." VERBATIM SOURCES ${test_SRCS})
add_custom_target(test "${MAINFOLDER}/bin/${test_BIN}" DEPENDS ${test_BIN} COMMENT "Executing unit tests..." VERBATIM SOURCES ${test_SRCS})
