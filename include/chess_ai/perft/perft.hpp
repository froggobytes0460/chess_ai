#ifndef CHESS_AI_PERFT_PERFT_HPP
#define CHESS_AI_PERFT_PERFT_HPP

#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move.hpp>
#include <cstdint>
#include <utility>
#include <vector>

// Performance test.
auto perft(Position& position, int depth) -> uint64_t;

auto divide(Position& position, int depth)
    -> std::vector<std::pair<Move, uint64_t>>;

#endif  // !CHESS_AI_PERFT_PERFT_HPP
