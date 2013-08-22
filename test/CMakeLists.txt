INCLUDE_DIRECTORIES (
  "${MAINFOLDER}/src/data"
  "${MAINFOLDER}/src/dicom"
  "${MAINFOLDER}/src/view"
  "${MAINFOLDER}/src/gui"
  "${MAINFOLDER}/src/processing"
  )

FILE (GLOB_RECURSE test_SRCS *.cpp *.cxx *.cc *.C *.c *.h *.hpp)
SET (test_LIBS ${Qt_LIBRARIES} ${UnitTestPlusPlus_LIBRARIES} ${Cerebra_LIBRARIES} vtkHybrid vtkCommon)
SET (test_BIN ${PROJECT_NAME}-unittests)

ADD_EXECUTABLE(${test_BIN} ${test_SRCS})
TARGET_LINK_LIBRARIES(${test_BIN} ${test_LIBS})

ADD_CUSTOM_TARGET(check ALL "${MAINFOLDER}/bin/${test_BIN}" DEPENDS ${test_BIN} COMMENT "Executing unit tests..." VERBATIM SOURCES ${test_SRCS})
ADD_CUSTOM_TARGET(test "${MAINFOLDER}/bin/${test_BIN}" DEPENDS ${test_BIN} COMMENT "Executing unit tests..." VERBATIM SOURCES ${test_SRCS})
