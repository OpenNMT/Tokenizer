cmake_minimum_required(VERSION 3.1.0)

# Set policy for setting the MSVC runtime library for static MSVC builds
if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()

project(OpenNMTTokenizer)

option(WITH_ICU "Compile with ICU" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(MSVC)
  if(NOT BUILD_SHARED_LIBS)
    if(CMAKE_VERSION VERSION_LESS "3.15.0")
      message(FATAL_ERROR "Use CMake 3.15 or later when setting BUILD_SHARED_LIBS to OFF")
    endif()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

set(INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}
  )

set(PUBLIC_HEADERS
  include/onmt/Alphabet.h
  include/onmt/AnnotatedToken.h
  include/onmt/BPE.h
  include/onmt/BPELearner.h
  include/onmt/CaseModifier.h
  include/onmt/ITokenizer.h
  include/onmt/SpaceTokenizer.h
  include/onmt/SubwordEncoder.h
  include/onmt/SubwordLearner.h
  include/onmt/Tokenizer.h
  )

set(SOURCES
  src/Alphabet.cc
  src/AnnotatedToken.cc
  src/BPE.cc
  src/BPELearner.cc
  src/CaseModifier.cc
  src/ITokenizer.cc
  src/SpaceTokenizer.cc
  src/SubwordEncoder.cc
  src/SubwordLearner.cc
  src/Tokenizer.cc
  src/unicode/Unicode.cc
  )

list(APPEND LINK_LIBRARIES "")

if (WITH_ICU)
  find_package(ICU REQUIRED)
  add_definitions(-DWITH_ICU)
  list(APPEND INCLUDE_DIRECTORIES ${ICU_INCLUDE_DIRS})
  list(APPEND LINK_LIBRARIES ${ICU_LIBRARIES})
else()
  list(APPEND SOURCES src/unicode/Data.cc)
endif()

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

  find_library(SP_TRAIN_LIBRARY NAMES sentencepiece_train)
  if(SP_TRAIN_LIBRARY)
    find_package(Threads)
    add_definitions(-DWITH_SP_TRAIN)
    message(STATUS "Found sentencepiece_train: ${SP_TRAIN_LIBRARY}")
    list(APPEND PUBLIC_HEADERS include/onmt/SPMLearner.h)
    list(APPEND SOURCES src/SPMLearner.cc)
    list(APPEND LINK_LIBRARIES
      ${SP_TRAIN_LIBRARY}
      ${CMAKE_THREAD_LIBS_INIT})
  else()
    message(WARNING "sentencepiece_train not found: training SentencePiece models will not be supported")
  endif()

  file(STRINGS ${SP_INCLUDE_DIR}/sentencepiece_processor.h HAS_SAMPLE_ENCODE REGEX "SampleEncode")
  if(HAS_SAMPLE_ENCODE)
    add_definitions(-DSP_HAS_SAMPLE_ENCODE)
  endif()

  file(STRINGS ${SP_INCLUDE_DIR}/sentencepiece_processor.h HAS_VOCAB_RESTRICTION REGEX "SetVocabulary")
  if(HAS_VOCAB_RESTRICTION)
    add_definitions(-DSP_HAS_VOCAB_RESTRICTION)
  endif()
endif()

add_library(${PROJECT_NAME} ${SOURCES})
include(GNUInstallDirs)
include(GenerateExportHeader)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
generate_export_header(${PROJECT_NAME} EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/onmt/${PROJECT_NAME_LOWER}_export.h)
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIRECTORIES})
target_link_libraries(${PROJECT_NAME} ${LINK_LIBRARIES})

if (NOT LIB_ONLY)
  add_subdirectory(cli)
  add_subdirectory(test)
endif()

install(
  TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
install(FILES
  ${PUBLIC_HEADERS} "${PROJECT_BINARY_DIR}/onmt/${PROJECT_NAME_LOWER}_export.h"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/onmt"
  )
install(FILES
  include/onmt/unicode/Unicode.h
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/onmt/unicode"
  )
