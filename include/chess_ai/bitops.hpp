#ifndef BITOPS_HPP
#define BITOPS_HPP

#include <bit>
#include <chess_ai/types.hpp>

// Sets the square bit on a bitboard.
inline void setBit(BitBoard& bb, Square sq) {
  bb |= (1ULL << static_cast<unsigned>(sq));
}

// Unsets the square bit on a bitboard.
inline void unsetBit(BitBoard& bb, Square sq) {
  bb &= ~(1ULL << static_cast<unsigned>(sq));
}

// Gets the status of the bit on the bitboard.
[[nodiscard]] inline auto getBit(BitBoard bb, Square sq) -> bool {
  return ((bb >> static_cast<unsigned>(sq)) & 1ULL) != 0;
}

// Count the active bits on the bitboard.
[[nodiscard]] inline auto popCount(BitBoard bb) -> int {
  return std::popcount(bb);
}

// Returns the square position of the MSB of bitboard. Returns `Square::NONE` if
// bb == 0.
[[nodiscard]] inline auto getMSB(BitBoard bb) -> Square {
  if (bb == 0) {
    return Square::NONE;
  }
  return static_cast<Square>(63 - std::countl_zero(bb));
}

// Returns the square position of the LSB of bitboard. Returns `Square::NONE` if
// bb == 0.
[[nodiscard]] inline auto getLSB(BitBoard bb) -> Square {
  if (bb == 0) {
    return Square::NONE;
  }
  return static_cast<Square>(std::countr_zero(bb));
}

// Pops the LSB from the bitboard and returns it. Returns `Square::NONE` if bb
// == 0.
// NOTE: The function is safe since is guards underflow.
inline auto popLSB(BitBoard& bb) -> Square {
  Square lsb = getLSB(bb);
  if (lsb != Square::NONE) {
    bb &= bb - 1;
  }
  return lsb;
}

#endif  // !BITOPS_HPP
