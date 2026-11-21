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

# Test files mirror the engine modules: tests/<module>/test_*.cpp.

# core
add_chess_test(test_move ./core/test_move.cpp)

# board
add_chess_test(test_fen ./board/test_fen.cpp)
add_chess_test(test_make_move ./board/test_make_move.cpp)
add_chess_test(test_repetition ./board/test_repetition.cpp)
add_chess_test(test_insufficient_material
               ./board/test_insufficient_material.cpp)

# movegen
add_chess_test(test_attacks ./movegen/test_attacks.cpp)
add_chess_test(test_attacks_sliding ./movegen/test_attacks_sliding.cpp)
add_chess_test(test_movegen ./movegen/test_movegen.cpp)
add_chess_test(test_in_check ./movegen/test_in_check.cpp)

# perft
add_chess_test(test_perft ./perft/test_perft.cpp)

# eval
add_chess_test(test_eval ./eval/test_eval.cpp)

# search
add_chess_test(test_search ./search/test_search.cpp)
