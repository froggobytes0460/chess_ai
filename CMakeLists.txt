cmake_minimum_required(VERSION 3.28)
project(ChessAI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(generate_tests "Builds the project tests" OFF)

add_library(compiler_flags INTERFACE)
target_compile_options(compiler_flags INTERFACE -Wall -Wextra -Werror)

if(APPLE)
  execute_process(
    COMMAND xcrun --sdk macosx --show-sdk-path
    OUTPUT_VARIABLE CHESS_AI_SDK_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  target_compile_options(compiler_flags INTERFACE -isysroot
                                                  "${CHESS_AI_SDK_PATH}")
endif()

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.hpp"
     "src/*.h")
list(REMOVE_ITEM ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_library(chess_engine STATIC ${ENGINE_SOURCES})
target_include_directories(chess_engine
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(chess_engine PUBLIC compiler_flags)

add_executable(main src/main.cpp)
target_link_libraries(main PRIVATE chess_engine compiler_flags)

if(generate_tests)
  message(STATUS "Test suite enabled, adding tests/ directory...")
  add_subdirectory(tests/)
endif()
