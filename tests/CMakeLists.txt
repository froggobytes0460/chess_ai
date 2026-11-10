include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.18.0)
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# Registers a GTest executable: builds test_source, links chess_engine +
# gtest_main, discovers cases.
function(add_chess_test test_name test_source)
  add_executable(${test_name} ${test_source})
  target_link_libraries(${test_name} PRIVATE chess_engine compiler_flags
                                             GTest::gtest_main)
  gtest_discover_tests(${test_name})
endfunction()

add_chess_test(test_fen ./test_fen.cpp)
add_chess_test(test_move ./test_move.cpp)
add_chess_test(test_attacks ./test_attacks.cpp)
add_chess_test(test_movegen ./test_movegen.cpp)
