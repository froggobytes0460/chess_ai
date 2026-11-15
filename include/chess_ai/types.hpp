#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>

// The chess board represented by each bit of a 64-bit (unsigned) integer.
using BitBoard = uint64_t;

// Type of piece occupied on a square.
enum class PieceType : uint8_t { KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN };

// The players of the game.
enum class Color : uint8_t { WHITE, BLACK };

enum class Piece : uint8_t {
  NONE,
  WHITE_KING,
  WHITE_QUEEN,
  WHITE_ROOK,
  WHITE_BISHOP,
  WHITE_KNIGHT,
  WHITE_PAWN,
  BLACK_KING,
  BLACK_QUEEN,
  BLACK_ROOK,
  BLACK_BISHOP,
  BLACK_KNIGHT,
  BLACK_PAWN,
};

// Enum indicating each bit's square position on a bitboard.
enum class Square : int8_t {
  NONE = -1,
  A1,
  B1,
  C1,
  D1,
  E1,
  F1,
  G1,
  H1,
  A2,
  B2,
  C2,
  D2,
  E2,
  F2,
  G2,
  H2,
  A3,
  B3,
  C3,
  D3,
  E3,
  F3,
  G3,
  H3,
  A4,
  B4,
  C4,
  D4,
  E4,
  F4,
  G4,
  H4,
  A5,
  B5,
  C5,
  D5,
  E5,
  F5,
  G5,
  H5,
  A6,
  B6,
  C6,
  D6,
  E6,
  F6,
  G6,
  H6,
  A7,
  B7,
  C7,
  D7,
  E7,
  F7,
  G7,
  H7,
  A8,
  B8,
  C8,
  D8,
  E8,
  F8,
  G8,
  H8,
};

// The castling rights listed as bit positions on an unsigned integer.
enum class CastlingRights : uint8_t {
  WHITE_KINGSIDE = 1 << 0,
  BLACK_KINGSIDE = 1 << 1,
  WHITE_QUEENSIDE = 1 << 2,
  BLACK_QUEENSIDE = 1 << 3,
};

[[nodiscard]] constexpr auto makePiece(Color col, PieceType pt) -> Piece {
  return static_cast<Piece>(1 + (static_cast<uint8_t>(col) * 6) +
                            static_cast<uint8_t>(pt));
}
[[nodiscard]] constexpr auto colorOf(Piece piece) -> Color {
  return static_cast<Color>((static_cast<uint8_t>(piece) - 1) / 6);
}
[[nodiscard]] constexpr auto typeOf(Piece piece) -> PieceType {
  return static_cast<PieceType>((static_cast<uint8_t>(piece) - 1) % 6);
}

// Returns the other player.
[[nodiscard]] constexpr auto opposite(Color color) -> Color {
  return color == Color::WHITE ? Color::BLACK : Color::WHITE;
}

#endif  // !TYPES_HPP
