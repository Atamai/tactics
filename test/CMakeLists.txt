file(GLOB_RECURSE test_SRCS *.cpp *.cxx *.cc *.C *.c *.h *.hpp)
set(test_LIBS ${Qt_LIBRARIES} ${UnitTestPlusPlus_LIBRARIES})
set(test_BIN ${PROJECT_NAME}-unittests)

configure_file(TestData.h.in "${CMAKE_CURRENT_BINARY_DIR}/TestData.h" @ONLY)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_executable(${test_BIN} ${test_SRCS})
target_link_libraries(${test_BIN} ${test_LIBS} cbElectrode)

add_custom_target(check ALL "${MAINFOLDER}/bin/${test_BIN}" DEPENDS ${test_BIN} COMMENT "Executing unit tests..." VERBATIM SOURCES ${test_SRCS})
