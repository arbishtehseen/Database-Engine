cmake_minimum_required(VERSION 3.10)
project(MiniDB CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT MSVC)
  add_compile_options(-Wall -Wextra -Werror -pedantic)
endif()

# Optional: cmake -DSANITIZE=ON ..
option(SANITIZE "Build with address and undefined sanitizers" OFF)
if(SANITIZE AND NOT MSVC)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

# The repo keeps files both in the root and in include/, src/, tests/,
# so we look in all of them.
file(GLOB ALL_CPP
  ${CMAKE_SOURCE_DIR}/*.cpp
  ${CMAKE_SOURCE_DIR}/src/*.cpp)

# Split: main.cpp, test_*.cpp, and everything else (the engine library)
set(ENGINE_SOURCES "")
set(MAIN_SOURCE "")
foreach(f ${ALL_CPP})
  get_filename_component(fname ${f} NAME)
  if(fname STREQUAL "main.cpp")
    set(MAIN_SOURCE ${f})
  elseif(NOT fname MATCHES "^test_")
    list(APPEND ENGINE_SOURCES ${f})
  endif()
endforeach()

add_library(minidb_lib ${ENGINE_SOURCES})
target_include_directories(minidb_lib PUBLIC
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src)

if(MAIN_SOURCE)
  add_executable(minidb ${MAIN_SOURCE})
  target_link_libraries(minidb minidb_lib)
endif()

# One executable per test_*.cpp (root or tests/), all registered with ctest
enable_testing()
file(GLOB TEST_SOURCES
  ${CMAKE_SOURCE_DIR}/test_*.cpp
  ${CMAKE_SOURCE_DIR}/tests/test_*.cpp)
foreach(test_src ${TEST_SOURCES})
  get_filename_component(test_name ${test_src} NAME_WE)
  if(NOT TARGET ${test_name})
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} minidb_lib)
    add_test(NAME ${test_name} COMMAND ${test_name})
  else()
    message(WARNING "Duplicate test name ${test_name} (${test_src}) skipped")
  endif()
endforeach()