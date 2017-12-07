enable_testing()
find_package(GTest)

if (GTEST_FOUND)
  find_package(Threads)

  add_executable(onmt_tokenizer_test test.cc)
  target_include_directories(onmt_tokenizer_test PUBLIC
    ${GTEST_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
    )
  target_link_libraries(onmt_tokenizer_test
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${PROJECT_NAME}
    )
  add_test(TokenizerTest onmt_tokenizer_test "${CMAKE_CURRENT_LIST_DIR}/data/")
endif()
