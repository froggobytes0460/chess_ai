#ifndef CHESS_AI_SEARCH_SEARCH_HPP
#define CHESS_AI_SEARCH_SEARCH_HPP

#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move.hpp>
#include <cstdint>

struct SearchResult {
  uint64_t nodes{};
  int32_t score{};
  Move best_move{};  // NOLINT(readability-redundant-member-init): GCC
                     // -Wmissing-field-initializers on designated init
  int16_t depth{};
};

auto search(Position& pos, int max_depth) -> SearchResult;

#endif  // !CHESS_AI_SEARCH_SEARCH_HPP
