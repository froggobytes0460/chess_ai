#include <algorithm>
#include <chess_ai/core/bitops.hpp>
#include <chess_ai/core/types.hpp>
#include <chess_ai/eval/eval.hpp>
#include <cstdint>

#include "pst.hpp"

auto evaluate(const Position& pos) -> int32_t {
  int32_t mg{};
  int32_t eg{};
  int32_t phase{};
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
      phase += popCount(bb) * phaseWeight(pt);
      while (bb != 0) {
        const Square sq{popLSB(bb)};
        mg += sign * mgScore(col, pt, sq);
        eg += sign * egScore(col, pt, sq);
      }
    }
  }
  // Blend middlegame and endgame scores by remaining material; promotions can
  // push `phase` past the starting total.
  phase = std::min(phase, MAX_PHASE);
  int32_t score{((mg * phase) + (eg * (MAX_PHASE - phase))) / MAX_PHASE};
  if (pos.sideToMove() == Color::BLACK) {
    score = -score;
  }
  return score;
}
