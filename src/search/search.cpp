#include <algorithm>
#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move.hpp>
#include <chess_ai/core/move_list.hpp>
#include <chess_ai/core/types.hpp>
#include <chess_ai/eval/eval.hpp>
#include <chess_ai/movegen/movegen.hpp>
#include <chess_ai/search/search.hpp>
#include <cstdint>

namespace {
constexpr int32_t INF{32000};
constexpr int32_t MATE{31000};

auto quiescence(Position& pos, int32_t alpha, int32_t beta, int ply,
                uint64_t& nodes) -> int32_t {
  ++nodes;
  const int32_t stand_pat = evaluate(pos);
  if (stand_pat >= beta) {
    return stand_pat;
  }
  alpha = std::max(stand_pat, alpha);
  int32_t best = stand_pat;

  MoveList capture_moves;
  generateCaptureMoves(pos, capture_moves);

  for (const Move move : capture_moves) {
    const MoveState state = pos.makeMove(move);
    if (inCheck(pos, opposite(pos.sideToMove()))) {
      pos.undoMove(move, state);
      continue;
    }
    const int32_t score = -quiescence(pos, -beta, -alpha, ply + 1, nodes);
    pos.undoMove(move, state);

    best = std::max(score, best);
    alpha = std::max(score, alpha);
    if (score >= beta) {
      break;
    }
  }

  return best;
}

auto negamax(Position& pos, int depth, int32_t alpha, int32_t beta, int ply,
             uint64_t& nodes) -> int32_t {
  ++nodes;
  if (pos.isRepetition() || pos.isInsufficientMaterial() ||
      pos.halfMoveClock() >= 100) {
    return 0;
  }
  if (depth == 0) {
    return quiescence(pos, alpha, beta, ply, nodes);
  }
  int32_t best = -INF;
  bool legal_moves_left = false;
  MoveList moves;
  generatePseudoLegalMoves(pos, moves);

  for (const Move move : moves) {
    const MoveState state = pos.makeMove(move);
    if (inCheck(pos, opposite(pos.sideToMove()))) {
      pos.undoMove(move, state);
      continue;
    }
    legal_moves_left = true;
    const int32_t score =
        -negamax(pos, depth - 1, -beta, -alpha, ply + 1, nodes);
    pos.undoMove(move, state);
    best = std::max(score, best);
    alpha = std::max(score, alpha);
    if (score >= beta) {
      break;
    }
  }
  if (!legal_moves_left) {
    return inCheck(pos, pos.sideToMove()) ? -MATE + ply : 0;
  }
  return best;
}

auto rootSearch(Position& pos, int depth, uint64_t& nodes) -> SearchResult {
  SearchResult result{.score = -INF, .depth = static_cast<int16_t>(depth)};

  int32_t alpha = -INF;
  MoveList moves;
  bool legal_moves_left = false;
  generatePseudoLegalMoves(pos, moves);

  for (const Move move : moves) {
    const MoveState state = pos.makeMove(move);
    if (inCheck(pos, opposite(pos.sideToMove()))) {
      pos.undoMove(move, state);
      continue;
    }
    legal_moves_left = true;
    const int32_t score = -negamax(pos, depth - 1, -INF, -alpha, 1, nodes);
    pos.undoMove(move, state);
    if (score > result.score) {
      result.score = score;
      result.best_move = move;
    }
    alpha = std::max(alpha, score);
  }
  if (!legal_moves_left) {
    result.score = inCheck(pos, pos.sideToMove()) ? -MATE : 0;
  }
  return result;
}
}  // namespace

auto search(Position& pos, int max_depth) -> SearchResult {
  SearchResult result{};
  uint64_t nodes{};
  for (int d = 1; d <= max_depth; ++d) {
    result = rootSearch(pos, d, nodes);
    result.nodes = nodes;
  }
  return result;
}
