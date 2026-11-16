add_executable(magic_gen ./magic_gen.cpp)
target_link_libraries(magic_gen PRIVATE chess_engine compiler_flags)

add_executable(perft_bench ./perft_bench.cpp)
target_link_libraries(perft_bench PRIVATE chess_engine compiler_flags)
