cmake_minimum_required(VERSION 3.20)
project(dread_kings VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(dread_kings
  src/core/dread_kings.cpp
  src/discovery/design_vector.cpp
  src/discovery/search.cpp
  src/robotics/robot_model.cpp
  src/toolchain/compiler_pipeline.cpp
  src/toolchain/clang_rewriter.cpp
  src/verification/verification.cpp
  src/toolchain/toolchain.cpp
  src/verification/concurrency_king.cpp
)
target_include_directories(dread_kings PUBLIC include)
target_compile_features(dread_kings PUBLIC cxx_std_23)
if(MSVC)
  target_compile_options(dread_kings PRIVATE /W4 /permissive-)
else()
  target_compile_options(dread_kings PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

add_executable(dread-kings examples/dread_kings.cpp)
target_link_libraries(dread-kings PRIVATE dread_kings)

include(CTest)
if(BUILD_TESTING)
  add_executable(dread_tests tests/test_core.cpp)
  target_link_libraries(dread_tests PRIVATE dread_kings)
  add_test(NAME dread_core COMMAND dread_tests)

  add_executable(dread_kings_pipeline_test tests/dread_kings_pipeline_test.cpp)
  target_link_libraries(dread_kings_pipeline_test PRIVATE dread_kings)
  add_test(NAME dread_kings_pipeline COMMAND dread_kings_pipeline_test)

  add_executable(clang_rewriter_test tests/clang_rewriter_test.cpp)
  target_link_libraries(clang_rewriter_test PRIVATE dread_kings)
  add_test(NAME clang_rewriter COMMAND clang_rewriter_test)
endif()
