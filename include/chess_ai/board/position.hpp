#ifndef CHESS_AI_BOARD_POSITION_HPP
#define CHESS_AI_BOARD_POSITION_HPP

#include <array>
#include <chess_ai/core/move.hpp>
#include <chess_ai/core/types.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

// State destroyed by `Position::makeMove`, needed by `Position::undoMove` to
// restore the previous position.
struct MoveState {
  uint64_t zobrist_key;
  uint16_t half_move_clock;
  uint8_t castling_rights_bits;
  Square en_passant;
  Piece captured;  // `Piece::NONE` if the move was not a capture
};

class Position {
 private:
  std::array<uint64_t, 1024> history_;
  std::array<std::array<BitBoard, 6>, 2> pieces_;
  std::array<Piece, 64> board_;
  std::array<BitBoard, 2> occupied_;
  BitBoard occupied_all_;
  uint64_t zobrist_key_;
  size_t history_count_;
  uint16_t full_move_counter_;
  uint16_t half_move_clock_;
  uint8_t castling_rights_bits_;
  Color side_to_move_;
  Square en_passant_;

  void putPiece(Piece piece, Square sq);
  void removePiece(Square sq);
  void movePiece(Square from, Square to);

  // Setters that keep `zobrist_key_` in sync with the changed field.
  void setEnPassant(Square sq);
  void setCastlingRights(uint8_t rights);

 public:
  Position();
  explicit Position(std::string_view fen);

  [[nodiscard]] auto toFEN() const -> std::string;
  [[nodiscard]] auto computeZobristKey() const -> uint64_t;

  // Returns the bitboard of pieces of `type` belonging to `color`.
  [[nodiscard]] constexpr auto pieces(Color color, PieceType type) const
      -> BitBoard {
    return pieces_[static_cast<size_t>(color)][static_cast<size_t>(type)];
  }

  [[nodiscard]] constexpr auto pieceAt(Square sq) const -> Piece {
    return board_[static_cast<size_t>(sq)];
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

  // Returns castling rights bit storage as a 1-byte unsigned integer
  [[nodiscard]] constexpr auto castlingRightsBits() const -> uint8_t {
    return castling_rights_bits_;
  }

  // Returns counter of full moves
  [[nodiscard]] constexpr auto fullMoveCounter() const -> uint16_t {
    return full_move_counter_;
  }

  // Returns the half-move clock
  [[nodiscard]] constexpr auto halfMoveClock() const -> uint16_t {
    return half_move_clock_;
  }

  // Returns the incrementally maintained Zobrist key.
  [[nodiscard]] constexpr auto zobristKey() const -> uint64_t {
    return zobrist_key_;
  }

  [[nodiscard]] auto makeMove(Move move) -> MoveState;

  void undoMove(Move move, MoveState move_state);

  [[nodiscard]] auto isRepetition() const -> bool;
};

#endif  // !CHESS_AI_BOARD_POSITION_HPP
