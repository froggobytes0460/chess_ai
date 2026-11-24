#ifndef CHESS_AI_CORE_MOVE_LIST_HPP
#define CHESS_AI_CORE_MOVE_LIST_HPP

#include <array>
#include <cassert>
#include <chess_ai/core/move.hpp>
#include <cstddef>

// Fixed-size move container, no heap allocation. Capacity covers the
// worst-case legal move count for any reachable chess position (218).
class MoveList {
 public:
  static constexpr size_t CAPACITY = 256;

 private:
  std::array<Move, CAPACITY> moves_{};
  size_t count_ = 0;

 public:
  constexpr void push(Move move) {
    assert(count_ < CAPACITY);
    moves_[count_++] = move;
  }

  [[nodiscard]] constexpr auto size() const -> size_t { return count_; }

  [[nodiscard]] constexpr auto begin() const -> const Move* {
    return moves_.data();
  }

  [[nodiscard]] constexpr auto end() const -> const Move* {
    return moves_.data() + count_;
  }

  [[nodiscard]] constexpr auto operator[](size_t index) const -> Move {
    return moves_[index];
  }
  [[nodiscard]] constexpr auto operator[](size_t index) -> Move& {
    return moves_[index];
  }
};

#endif  // !CHESS_AI_CORE_MOVE_LIST_HPP
