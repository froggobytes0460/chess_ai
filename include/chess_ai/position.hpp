#ifndef POSITION_HPP
#define POSITION_HPP

#include <array>
#include <chess_ai/types.hpp>
#include <cstddef>
#include <cstdint>
#include <string_view>

class Position {
 private:
  std::array<std::array<BitBoard, 6>, 2> pieces_;
  std::array<BitBoard, 2> occupied_;
  BitBoard occupied_all_;
  uint64_t zobrist_key_;
  uint16_t full_move_counter_;
  uint16_t half_move_clock_;
  uint8_t castling_rights_bits_;
  Color side_to_move_;
  Square en_passant_;

  friend class PositionTest;

 public:
  Position();
  explicit Position(std::string_view fen);

  [[nodiscard]] auto toFEN() -> std::string;
  [[nodiscard]] auto computeZobristKey() const -> uint64_t;

  // Returns the bitboard of pieces of `type` belonging to `color`.
  [[nodiscard]] constexpr auto pieces(Color color, PieceType type) const
      -> BitBoard {
    return pieces_[static_cast<size_t>(color)][static_cast<size_t>(type)];
  }

  // Returns the combined occupancy bitboard for `color`.
  [[nodiscard]] constexpr auto occupied(Color color) const -> BitBoard {
    return occupied_[static_cast<size_t>(color)];
  }

  // Returns the combined occupancy bitboard for both colors.
  [[nodiscard]] constexpr auto occupiedAll() const -> BitBoard {
    return occupied_all_;
  }

  // Returns the current en-passant target square, or `Square::NONE`.
  [[nodiscard]] constexpr auto enPassantSquare() const -> Square {
    return en_passant_;
  }

  // Returns the side to move.
  [[nodiscard]] constexpr auto sideToMove() const -> Color {
    return side_to_move_;
  }
};

#endif  // !POSITION_HPP
