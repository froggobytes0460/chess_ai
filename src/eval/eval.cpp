#include <chess_ai/core/bitops.hpp>
#include <chess_ai/core/types.hpp>
#include <chess_ai/eval/eval.hpp>
#include <cstdint>

#include "pst.hpp"

auto evaluate(const Position& pos) -> int32_t {
  int32_t score{};
  for (const Color col : {Color::WHITE, Color::BLACK}) {
    const int32_t sign{col == Color::WHITE ? 1 : -1};
    for (const PieceType pt : {
             PieceType::KING,
             PieceType::QUEEN,
             PieceType::ROOK,
             PieceType::BISHOP,
             PieceType::KNIGHT,
             PieceType::PAWN,
         }) {
      BitBoard bb{pos.pieces(col, pt)};
      while (bb != 0) {
        score += sign * mgScore(col, pt, popLSB(bb));
      }
    }
  }
  if (pos.sideToMove() == Color::BLACK) {
    score = -score;
  }
  return score;
}
