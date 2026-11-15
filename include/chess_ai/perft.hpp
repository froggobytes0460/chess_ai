#ifndef PERFT_HPP
#define PERFT_HPP

#include <chess_ai/move.hpp>
#include <chess_ai/position.hpp>
#include <cstdint>
#include <utility>
#include <vector>

// Performance test.
auto perft(Position& position, int depth) -> uint64_t;

auto divide(Position& position, int depth)
    -> std::vector<std::pair<Move, uint64_t>>;

#endif  // !PERFT_HPP
