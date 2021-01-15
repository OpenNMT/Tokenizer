enable_testing()

set(BUILD_GMOCK OFF CACHE BOOL "Build googlemock")
set(INSTALL_GTEST OFF CACHE BOOL "Install googletest")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/googletest ${CMAKE_CURRENT_BINARY_DIR}/googletest)

add_executable(onmt_tokenizer_test test.cc)
target_link_libraries(onmt_tokenizer_test
  ${PROJECT_NAME}
  gtest_main
  )

add_test(TokenizerTest onmt_tokenizer_test "${CMAKE_CURRENT_LIST_DIR}/data/")
