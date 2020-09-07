set(CXXOPTS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/cxxopts/include)

add_executable(tokenize
  tokenize.cc
  )
target_include_directories(tokenize
  PRIVATE ${CXXOPTS_INCLUDE_DIR}
  )
target_link_libraries(tokenize
  ${PROJECT_NAME}
  )

add_executable(subword_learn
  learn.cc
  )
target_include_directories(subword_learn
  PRIVATE ${CXXOPTS_INCLUDE_DIR}
  )
target_link_libraries(subword_learn
  ${PROJECT_NAME}
  )

add_executable(detokenize
  detokenize.cc
  )
target_include_directories(detokenize
  PRIVATE ${CXXOPTS_INCLUDE_DIR}
  )
target_link_libraries(detokenize
  ${PROJECT_NAME}
  )

install(
  TARGETS tokenize detokenize
  DESTINATION bin/
  )
