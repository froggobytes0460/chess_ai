#include <algorithm>
#include <array>
#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move.hpp>
#include <chess_ai/core/move_list.hpp>
#include <chess_ai/core/types.hpp>
#include <chess_ai/eval/eval.hpp>
#include <chess_ai/movegen/movegen.hpp>
#include <chess_ai/search/search.hpp>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace {
constexpr int32_t INF{32000};
constexpr int32_t MATE{31000};
constexpr std::array<int32_t, 6> ORDER_VALUE{10, 9, 5, 3, 3, 1};
constexpr int32_t PV_SCORE{1'000'000};
constexpr int32_t PROMO_BONUS{10'000};

using ScoreList = std::array<int32_t, MoveList::CAPACITY>;

constexpr auto orderValue(PieceType type) -> int32_t {
  return ORDER_VALUE[static_cast<size_t>(type)];
}
auto scoreMove(const Position& pos, Move move) -> int32_t {
  int32_t score{};
  if (move.isCapture()) {
    const PieceType victim = move.flag() == MoveFlag::EN_PASSANT
                                 ? PieceType::PAWN
                                 : typeOf(pos.pieceAt(move.to()));
    const PieceType attacker = typeOf(pos.pieceAt(move.from()));
    score += (orderValue(victim) * 10) - orderValue(attacker);
  }
  if (move.isPromotion()) {
    score += PROMO_BONUS + orderValue(move.promotionType());
  }
  return score;
}

void scoreMoves(const Position& pos, const MoveList& move_list,
                ScoreList& score_list, Move pv_move) {
  std::ranges::transform(
      move_list, score_list.begin(), [&](Move move) -> int32_t {
        return move == pv_move ? PV_SCORE : scoreMove(pos, move);
      });
}
void pickNext(MoveList& move_list, ScoreList& score_list, size_t i) {
  // NOLINTBEGIN(readability-qualified-auto) Pointer issues
  const auto first = score_list.begin() + static_cast<ptrdiff_t>(i);
  const auto last =
      score_list.begin() + static_cast<ptrdiff_t>(move_list.size());
  // NOLINTEND(readability-qualified-auto)
  const auto j =
      static_cast<size_t>(std::max_element(first, last) - score_list.begin());
  std::swap(score_list[i], score_list[j]);
  std::swap(move_list[i], move_list[j]);
}

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
  ScoreList scores;
  scoreMoves(pos, capture_moves, scores, Move{});

  for (size_t i = 0; i < capture_moves.size(); ++i) {
    pickNext(capture_moves, scores, i);
    const Move move = capture_moves[i];
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
  ScoreList scores;
  scoreMoves(pos, moves, scores, Move{});

  for (size_t i = 0; i < moves.size(); ++i) {
    pickNext(moves, scores, i);
    const Move move = moves[i];
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

auto rootSearch(Position& pos, int depth, uint64_t& nodes, Move pv_move)
    -> SearchResult {
  SearchResult result{.score = -INF, .depth = static_cast<int16_t>(depth)};

  int32_t alpha = -INF;
  MoveList moves;
  bool legal_moves_left = false;
  generatePseudoLegalMoves(pos, moves);
  ScoreList scores;
  scoreMoves(pos, moves, scores, pv_move);

  for (size_t i = 0; i < moves.size(); ++i) {
    pickNext(moves, scores, i);
    const Move move = moves[i];
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
    result = rootSearch(pos, d, nodes, result.best_move);
    result.nodes = nodes;
  }
  return result;
}
