# Python wrapper.
if (NOT DEFINED PYTHON_VERSION)
  set(PYTHON_VERSION "2.7")
endif()

find_package(PythonLibs ${PYTHON_VERSION})

if (PYTHONLIBS_FOUND)
  string(REPLACE "." "" PYTHON_VERSION_SHORT ${PYTHONLIBS_VERSION_STRING})
  string(SUBSTRING ${PYTHON_VERSION_SHORT} 0 2 PYTHON_VERSION_SHORT)

  find_package(Boost COMPONENTS python-py${PYTHON_VERSION_SHORT})

  if (Boost_PYTHON-PY${PYTHON_VERSION_SHORT}_FOUND)
    set(WITH_PYTHON_WRAPPER ON)
    set(PYTHON_MODULE_NAME "pyonmttok")

    add_library(${PYTHON_MODULE_NAME} Python.cc)
    set_target_properties(${PYTHON_MODULE_NAME} PROPERTIES PREFIX "")

    target_include_directories(${PYTHON_MODULE_NAME} PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${Boost_INCLUDE_DIRS}
      ${PYTHON_INCLUDE_DIRS}
      )
    target_link_libraries(${PYTHON_MODULE_NAME}
      ${PROJECT_NAME}
      ${Boost_LIBRARIES}
      ${PYTHON_LIBRARIES}
      )
  endif()
endif()

if (NOT WITH_PYTHON_WRAPPER)
  message(WARNING "Compilation without Python bindings")
endif()
