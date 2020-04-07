find_package(Boost REQUIRED COMPONENTS program_options)

include_directories(
  ${Boost_INCLUDE_DIRS}
  )

if (WIN32 AND NOT CYGWIN AND NOT Boost_USE_STATIC_LIBS)
  add_definitions(-DBOOST_ALL_NO_LIB) #Boost: Tells the config system not to automatically select which libraries to link against
  add_definitions(-DBOOST_ALL_DYN_LINK) #Boost: Forces all libraries that have separate source, to be linked as dll's rather than static libraries on Microsoft Windows
endif()

add_executable(tokenize
  tokenize.cc
  )
target_link_libraries(tokenize
  ${PROJECT_NAME}
  ${Boost_LIBRARIES}
  )

add_executable(subword_learn
  learn.cc
  )
target_link_libraries(subword_learn
  ${PROJECT_NAME}
  ${Boost_LIBRARIES}
  )

add_executable(detokenize
  detokenize.cc
  )
target_link_libraries(detokenize
  ${PROJECT_NAME}
  ${Boost_LIBRARIES}
  )

install(
  TARGETS tokenize detokenize
  DESTINATION bin/
  )
