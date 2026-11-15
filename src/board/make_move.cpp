#include <array>
#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move.hpp>
#include <chess_ai/core/types.hpp>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "zobrist_key.hpp"

namespace {
// Castling rights kept when a piece moves from or to each square; ANDed with
// `castling_rights_bits_` for both `from` and `to`. Bits: WK=1, BK=2, WQ=4,
// BQ=8.
constexpr std::array<uint8_t, 64> CASTLING_RIGHTS_MASK =
    []() -> std::array<uint8_t, 64> {
  std::array<uint8_t, 64> mask{};
  mask.fill(0b1111);
  mask[static_cast<size_t>(Square::E1)] = 0b1010;  // clears WK, WQ
  mask[static_cast<size_t>(Square::H1)] = 0b1110;  // clears WK
  mask[static_cast<size_t>(Square::A1)] = 0b1011;  // clears WQ
  mask[static_cast<size_t>(Square::E8)] = 0b0101;  // clears BK, BQ
  mask[static_cast<size_t>(Square::H8)] = 0b1101;  // clears BK
  mask[static_cast<size_t>(Square::A8)] = 0b0111;  // clears BQ
  return mask;
}();

// Castling rights left from `rights` after a piece moves from `from` to `to`.
constexpr auto castlingRightsAfterMove(uint8_t rights, Square from, Square to)
    -> uint8_t {
  return static_cast<uint8_t>(
      static_cast<unsigned>(rights) &
      static_cast<unsigned>(CASTLING_RIGHTS_MASK[static_cast<size_t>(from)]) &
      static_cast<unsigned>(CASTLING_RIGHTS_MASK[static_cast<size_t>(to)]));
}

constexpr auto offset(Square sq, int off) -> Square {
  return static_cast<Square>(static_cast<int>(sq) + off);
}

// Square offset of one rank toward the opponent, from `color`'s perspective.
constexpr auto forward(Color color) -> int {
  return color == Color::WHITE ? 8 : -8;
}

// Square of the pawn captured when `us` captures en passant onto `to`.
constexpr auto enPassantCaptureSquare(Square to, Color us) -> Square {
  return offset(to, -forward(us));
}

// Rook squares for a castling move whose king lands on `king_to`.
constexpr auto castlingRookMove(Move move) -> std::pair<Square, Square> {
  const Square king_to = move.to();
  if (move.flag() == MoveFlag::CASTLE_KINGSIDE) {
    return {offset(king_to, 1), offset(king_to, -1)};
  }
  return {offset(king_to, -2), offset(king_to, 1)};
}

}  // namespace

auto Position::makeMove(Move move) -> MoveState {
  const Color us = side_to_move_;
  const Square capture_sq = (move.flag() == MoveFlag::EN_PASSANT)
                                ? enPassantCaptureSquare(move.to(), us)
                                : move.to();
  const PieceType moving_type = typeOf(pieceAt(move.from()));

  const MoveState state{
      .zobrist_key = zobrist_key_,
      .half_move_clock = half_move_clock_,
      .castling_rights_bits = castling_rights_bits_,
      .en_passant = en_passant_,
      .captured = pieceAt(capture_sq),
  };

  setEnPassant(move.flag() == MoveFlag::DOUBLE_PUSH
                   ? offset(move.from(), forward(us))
                   : Square::NONE);

  if (move.isCapture()) {
    removePiece(capture_sq);
  }

  if (move.isPromotion()) {
    removePiece(move.from());
    putPiece(makePiece(us, move.promotionType()), move.to());
  } else {
    movePiece(move.from(), move.to());
  }

  if (move.isCastle()) {
    const auto [rook_from, rook_to] = castlingRookMove(move);
    movePiece(rook_from, rook_to);
  }

  setCastlingRights(
      castlingRightsAfterMove(castling_rights_bits_, move.from(), move.to()));

  if (moving_type == PieceType::PAWN || move.isCapture()) {
    half_move_clock_ = 0;
  } else {
    ++half_move_clock_;
  }

  if (us == Color::BLACK) {
    ++full_move_counter_;
  }

  side_to_move_ = opposite(us);
  zobrist_key_ ^= getZobristKeyForSideToMove();

  return state;
}

void Position::undoMove(Move move, MoveState move_state) {
  side_to_move_ = opposite(side_to_move_);
  Color us = side_to_move_;
  const Square capture_sq = (move.flag() == MoveFlag::EN_PASSANT)
                                ? enPassantCaptureSquare(move.to(), us)
                                : move.to();
  if (us == Color::BLACK) {
    --full_move_counter_;
  }

  if (move.isCastle()) {
    const auto [rook_from, rook_to] = castlingRookMove(move);
    movePiece(rook_to, rook_from);
  }

  if (move.isPromotion()) {
    removePiece(move.to());
    putPiece(makePiece(us, PieceType::PAWN), move.from());
  } else {
    movePiece(move.to(), move.from());
  }

  if (move.isCapture()) {
    putPiece(move_state.captured, capture_sq);
  }

  half_move_clock_ = move_state.half_move_clock;
  castling_rights_bits_ = move_state.castling_rights_bits;
  en_passant_ = move_state.en_passant;
  zobrist_key_ = move_state.zobrist_key;
}
