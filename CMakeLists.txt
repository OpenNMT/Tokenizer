cmake_minimum_required(VERSION 3.1.0)
project(OpenNMTTokenizer)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

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
  include/onmt/ITokenizer.h
  include/onmt/Tokenizer.h
  include/onmt/BPE.h
  include/onmt/CaseModifier.h
  include/onmt/SpaceTokenizer.h
  )

add_library(${PROJECT_NAME}
  src/BPE.cc
  src/CaseModifier.cc
  src/ITokenizer.cc
  src/SpaceTokenizer.cc
  src/Tokenizer.cc
  src/unicode/Data.cc
  src/unicode/Unicode.cc
  )

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIRECTORIES})

if (NOT LIB_ONLY)
  add_subdirectory(cli)
endif()

add_subdirectory(test)

install(
  TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION lib/
  LIBRARY DESTINATION lib/
  )
install(
  FILES ${PUBLIC_HEADERS}
  DESTINATION include/onmt/
  )
