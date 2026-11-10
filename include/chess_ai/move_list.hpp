#ifndef MOVE_LIST_HPP
#define MOVE_LIST_HPP

#include <array>
#include <chess_ai/move.hpp>
#include <cstddef>

// Fixed-size move container, no heap allocation. Capacity covers the
// worst-case legal move count for any reachable chess position (218).
class MoveList {
 private:
  static constexpr size_t CAPACITY = 256;

  std::array<Move, CAPACITY> moves_{};
  size_t count_ = 0;

 public:
  constexpr void push(Move move) { moves_[count_++] = move; }

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
};

#endif  // !MOVE_LIST_HPP
