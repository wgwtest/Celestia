set(UNIT_TEST_SOURCES
  array_view_test.cpp
  associativearray_test.cpp
  category_test.cpp
  constellation_test.cpp
  greek_test.cpp
  kepler_test.cpp
  logger_test.cpp
  mvc_boundary_test.cpp
  mvc_step2_contract_test.cpp
  mvc_step3_contract_test.cpp
  mvc_step4_directory_contract_test.cpp
  mvc_step5_frontend_contract_test.cpp
  mvc_step5_runtime_contract_test.cpp
  mvc_step5_view_frame_test.cpp
  mvc_step5_view_provider_test.cpp
  ranges_test.cpp
  stellarclass_test.cpp
  strnatcmp_test.cpp
  tokenizer_test.cpp)

if(USE_ICU)
  list(APPEND UNIT_TEST_SOURCES unicode_test.cpp)
else()
  list(APPEND UNIT_TEST_SOURCES formatnum_test.cpp)
endif()

if(WIN32)
  list(APPEND UNIT_TEST_SOURCES winutil_test.cpp)
endif()

test_case(unit "${UNIT_TEST_SOURCES}")
