cmake_minimum_required(VERSION 3.1.0)
project(OpenNMTTokenizer)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(WITH_PYTHON_BINDINGS "Build Python bindings" OFF)

set(CMAKE_CXX_STANDARD 11)

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

set(INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

set(PUBLIC_HEADERS
  include/onmt/Alphabet.h
  include/onmt/AnnotatedToken.h
  include/onmt/BPE.h
  include/onmt/CaseModifier.h
  include/onmt/ITokenizer.h
  include/onmt/SpaceTokenizer.h
  include/onmt/SubwordEncoder.h
  include/onmt/Tokenizer.h
  )

set(SOURCES
  src/Alphabet.cc
  src/AnnotatedToken.cc
  src/BPE.cc
  src/CaseModifier.cc
  src/ITokenizer.cc
  src/SpaceTokenizer.cc
  src/SubwordEncoder.cc
  src/Tokenizer.cc
  src/unicode/Data.cc
  src/unicode/Unicode.cc
  )

list(APPEND LINK_LIBRARIES "")

find_library(SP_LIBRARY NAMES sentencepiece)
find_path(SP_INCLUDE_DIR NAMES sentencepiece_processor.h)

if(NOT SP_LIBRARY OR NOT SP_INCLUDE_DIR)
  message(WARNING "sentencepiece not found; will not be supported")
else()
  message(STATUS "Found sentencepiece: ${SP_LIBRARY}")
  add_definitions(-DWITH_SP)
  list(APPEND PUBLIC_HEADERS include/onmt/SentencePiece.h)
  list(APPEND SOURCES src/SentencePiece.cc)
  list(APPEND INCLUDE_DIRECTORIES ${SP_INCLUDE_DIR})
  list(APPEND LINK_LIBRARIES ${SP_LIBRARY})
endif()

add_library(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIRECTORIES})
target_link_libraries(${PROJECT_NAME} ${LINK_LIBRARIES})

if (NOT LIB_ONLY)
  add_subdirectory(cli)
endif()

add_subdirectory(bindings)
add_subdirectory(test)

install(
  TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION lib/
  LIBRARY DESTINATION lib/
  )
install(FILES
  ${PUBLIC_HEADERS}
  DESTINATION include/onmt/
  )
install(FILES
  include/onmt/unicode/Unicode.h
  DESTINATION include/onmt/unicode
  )
