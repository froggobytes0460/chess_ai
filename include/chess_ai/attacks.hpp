#ifndef ATTACKS_HPP
#define ATTACKS_HPP

#include <array>
#include <chess_ai/bitops.hpp>
#include <chess_ai/types.hpp>

namespace detail {

// Computes the attack bitboard for a single square given a set of (file,
// rank) deltas, dropping any target that falls off the board.
constexpr auto computeAttacks(int square,
                              const std::array<std::pair<int, int>, 8>& deltas)
    -> BitBoard {
  BitBoard attacks = 0;
  int file = square % 8;
  int rank = square / 8;
  for (const auto& [delta_file, delta_rank] : deltas) {
    int target_file = file + delta_file;
    int target_rank = rank + delta_rank;
    if (target_file < 0 || target_file > 7 || target_rank < 0 ||
        target_rank > 7) {
      continue;
    }
    setBit(attacks, static_cast<Square>((target_rank * 8) + target_file));
  }
  return attacks;
}

constexpr std::array<std::pair<int, int>, 8> KNIGHT_DELTAS = {
    {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}},
};

constexpr std::array<std::pair<int, int>, 8> KING_DELTAS = {
    {{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}},
};

template <const std::array<std::pair<int, int>, 8>& deltas>
constexpr auto buildTable() -> std::array<BitBoard, 64> {
  std::array<BitBoard, 64> table{};
  for (int square = 0; square < 64; ++square) {
    // NOLINTNEXTLINE(readability-redundant-parentheses)
    table[square] = computeAttacks(square, deltas);
  }
  return table;
}

constexpr std::array<std::pair<int, int>, 2> WHITE_PAWN_ATTACK_DELTAS = {
    {{-1, 1}, {1, 1}},
};

constexpr std::array<std::pair<int, int>, 2> BLACK_PAWN_ATTACK_DELTAS = {
    {{-1, -1}, {1, -1}},
};

constexpr auto computePawnAttacks(
    int square, const std::array<std::pair<int, int>, 2>& deltas) -> BitBoard {
  BitBoard attacks = 0;
  int file = square % 8;
  int rank = square / 8;
  for (const auto& [delta_file, delta_rank] : deltas) {
    int target_file = file + delta_file;
    int target_rank = rank + delta_rank;
    if (target_file < 0 || target_file > 7 || target_rank < 0 ||
        target_rank > 7) {
      continue;
    }
    setBit(attacks, static_cast<Square>((target_rank * 8) + target_file));
  }
  return attacks;
}

template <const std::array<std::pair<int, int>, 2>& deltas>
constexpr auto buildPawnTable() -> std::array<BitBoard, 64> {
  std::array<BitBoard, 64> table{};
  for (int square = 0; square < 64; ++square) {
    // NOLINTNEXTLINE(readability-redundant-parentheses)
    table[square] = computePawnAttacks(square, deltas);
  }
  return table;
}

}  // namespace detail

namespace {
// Precomputed knight attack bitboards, indexed by `Square`.
constexpr std::array<BitBoard, 64> KNIGHT_ATTACKS =
    detail::buildTable<detail::KNIGHT_DELTAS>();

// Precomputed king attack bitboards, indexed by `Square`.
constexpr std::array<BitBoard, 64> KING_ATTACKS =
    detail::buildTable<detail::KING_DELTAS>();

// Precomputed pawn attack bitboards, indexed by `[color][square]`.
constexpr std::array<std::array<BitBoard, 64>, 2> PAWN_ATTACKS = {
    detail::buildPawnTable<detail::WHITE_PAWN_ATTACK_DELTAS>(),
    detail::buildPawnTable<detail::BLACK_PAWN_ATTACK_DELTAS>(),
};
}  // namespace

// Returns the precomputed knight attack bitboard for `sq`.
[[nodiscard]] constexpr auto knightAttacks(Square sq) -> BitBoard {
  return KNIGHT_ATTACKS[static_cast<unsigned>(sq)];
}

// Returns the precomputed king attack bitboard for `sq`.
[[nodiscard]] constexpr auto kingAttacks(Square sq) -> BitBoard {
  return KING_ATTACKS[static_cast<unsigned>(sq)];
}

// Returns the precomputed pawn attack (capture target) bitboard for `color`'s
// pawn on `sq`.
[[nodiscard]] constexpr auto pawnAttacks(Color color, Square sq) -> BitBoard {
  return PAWN_ATTACKS[static_cast<unsigned>(color)][static_cast<unsigned>(sq)];
}

#endif  // !ATTACKS_HPP
