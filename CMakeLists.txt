cmake_minimum_required(VERSION 3.20)
project(dread_lords VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(dreadlords
  src/core/dread_council.cpp
  src/discovery/design_vector.cpp
  src/discovery/search.cpp
  src/robotics/robot_model.cpp
  src/toolchain/compiler_pipeline.cpp
  src/toolchain/clang_rewriter.cpp
  src/verification/verification.cpp
  src/toolchain/toolchain.cpp
  src/verification/concurrency_lord.cpp
)
target_include_directories(dreadlords PUBLIC include)
target_compile_features(dreadlords PUBLIC cxx_std_23)
if(MSVC)
  target_compile_options(dreadlords PRIVATE /W4 /permissive-)
else()
  target_compile_options(dreadlords PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

add_executable(dreadctl examples/dreadctl.cpp)
target_link_libraries(dreadctl PRIVATE dreadlords)

include(CTest)
if(BUILD_TESTING)
  add_executable(dread_tests tests/test_core.cpp)
  target_link_libraries(dread_tests PRIVATE dreadlords)
  add_test(NAME dread_core COMMAND dread_tests)

  add_executable(council_pipeline_test tests/council_pipeline_test.cpp)
  target_link_libraries(council_pipeline_test PRIVATE dreadlords)
  add_test(NAME council_pipeline COMMAND council_pipeline_test)

  add_executable(clang_rewriter_test tests/clang_rewriter_test.cpp)
  target_link_libraries(clang_rewriter_test PRIVATE dreadlords)
  add_test(NAME clang_rewriter COMMAND clang_rewriter_test)
endif()
