add_executable(magic_gen ./magic_gen.cpp)
target_link_libraries(magic_gen PRIVATE chess_engine compiler_flags)
