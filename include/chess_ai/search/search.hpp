#ifndef CHESS_AI_SEARCH_SEARCH_HPP
#define CHESS_AI_SEARCH_SEARCH_HPP

#include <atomic>
#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move.hpp>
#include <chrono>
#include <cstdint>
#include <limits>

constexpr int MAX_DEPTH{12};

struct SearchLimits {
  int depth{MAX_DEPTH};
  uint64_t nodes{std::numeric_limits<uint64_t>::max()};
  std::chrono::steady_clock::time_point deadline{
      std::chrono::steady_clock::time_point::max(),
  };
};

struct SearchResult {
  uint64_t nodes{};
  int32_t score{};
  Move best_move{};  // NOLINT(readability-redundant-member-init): GCC
                     // -Wmissing-field-initializers on designated init
  int16_t depth{};
};

auto search(Position& pos, const SearchLimits& limits, std::atomic<bool>& stop)
    -> SearchResult;

#endif  // !CHESS_AI_SEARCH_SEARCH_HPP
