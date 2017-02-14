enable_testing()

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

file(GLOB files "*.raw")
foreach(file ${files})
  add_test(${file} sh ${CMAKE_CURRENT_SOURCE_DIR}/test.sh ${file} ${CMAKE_BINARY_DIR})
endforeach()
