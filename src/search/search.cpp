#include <algorithm>
#include <array>
#include <atomic>
#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move.hpp>
#include <chess_ai/core/move_list.hpp>
#include <chess_ai/core/types.hpp>
#include <chess_ai/eval/eval.hpp>
#include <chess_ai/movegen/movegen.hpp>
#include <chess_ai/search/search.hpp>
#include <chess_ai/search/tt.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

namespace {
constexpr int32_t INF{32000};
constexpr std::array<int32_t, 6> ORDER_VALUE{10, 9, 5, 3, 3, 1};
constexpr int32_t PV_SCORE{1'000'000};
constexpr int32_t PROMO_BONUS{10'000};
constexpr int32_t CAPTURE_BASE{100'000};
constexpr int32_t KILLER_1_SCORE{90'000};
constexpr int32_t KILLER_2_SCORE{80'000};
constexpr int32_t MAX_HISTORY{16'384};
constexpr int32_t MATE_BOUND{MATE - MAX_PLY};

// Mate scores are root-relative in search, node-relative in the TT.
constexpr auto toTT(int32_t score, int ply) -> int32_t {
  if (score >= MATE_BOUND) {
    return score + ply;
  }
  if (score <= -MATE_BOUND) {
    return score - ply;
  }
  return score;
}
constexpr auto fromTT(int32_t score, int ply) -> int32_t {
  if (score >= MATE_BOUND) {
    return score - ply;
  }
  if (score <= -MATE_BOUND) {
    return score + ply;
  }
  return score;
}

struct SearchContext {
  SearchLimits limits;
  TranspositionTable* tt{};
  std::atomic<bool>* stop{};
  uint64_t nodes{};
  bool can_stop{false};
  std::array<std::array<Move, 2>, MAX_PLY> killers{};
  std::array<std::array<std::array<int32_t, 64>, 64>, 2> history{};
};

using ScoreList = std::array<int32_t, MoveList::CAPACITY>;

constexpr auto orderValue(PieceType type) -> int32_t {
  return ORDER_VALUE[static_cast<size_t>(type)];
}
auto scoreMove(const Position& pos, const SearchContext& ctx, Move move,
               int ply) -> int32_t {
  int32_t score{};
  if (move.isCapture()) {
    const PieceType victim = move.flag() == MoveFlag::EN_PASSANT
                                 ? PieceType::PAWN
                                 : typeOf(pos.pieceAt(move.to()));
    const PieceType attacker = typeOf(pos.pieceAt(move.from()));
    score += CAPTURE_BASE + (orderValue(victim) * 10) - orderValue(attacker);
  }
  if (move.isPromotion()) {
    score += PROMO_BONUS + orderValue(move.promotionType());
  }
  if (move.isPromotion() || move.isCapture()) {
    return score;
  }

  const auto& killers = ctx.killers[static_cast<size_t>(ply)];
  if (move == killers[0]) {
    return KILLER_1_SCORE;
  }
  if (move == killers[1]) {
    return KILLER_2_SCORE;
  }
  return ctx.history[static_cast<size_t>(pos.sideToMove())][static_cast<size_t>(
      move.from())][static_cast<size_t>(move.to())];
}

auto isStopped(const SearchContext& ctx) -> bool {
  return ctx.can_stop && ctx.stop->load(std::memory_order_relaxed);
}

auto shouldStop(SearchContext& ctx) -> bool {
  if ((ctx.nodes & 2047) == 0 &&
      (ctx.nodes >= ctx.limits.nodes ||
       std::chrono::steady_clock::now() >= ctx.limits.deadline)) {
    ctx.stop->store(true, std::memory_order_relaxed);
  }
  return isStopped(ctx);
}
void scoreMoves(const Position& pos, const MoveList& move_list,
                ScoreList& score_list, Move pv_move, const SearchContext& ctx,
                int ply) {
  std::ranges::transform(
      move_list, score_list.begin(), [&](Move move) -> int32_t {
        return move == pv_move ? PV_SCORE : scoreMove(pos, ctx, move, ply);
      });
}
void pickNext(MoveList& move_list, ScoreList& score_list, size_t i) {
  // NOLINTBEGIN(readability-qualified-auto) Pointer arithmetic typing issues
  const auto first = score_list.begin() + static_cast<ptrdiff_t>(i);
  const auto last =
      score_list.begin() + static_cast<ptrdiff_t>(move_list.size());
  // NOLINTEND(readability-qualified-auto)
  const auto j =
      static_cast<size_t>(std::max_element(first, last) - score_list.begin());
  std::swap(score_list[i], score_list[j]);
  std::swap(move_list[i], move_list[j]);
}

void storeQuietCutoff(SearchContext& ctx, Color side, Move move, int depth,
                      int ply) {
  auto& killers = ctx.killers[static_cast<size_t>(ply)];
  if (killers[0] != move) {
    killers[1] = killers[0];
    killers[0] = move;
  }
  int32_t& h =
      ctx.history[static_cast<size_t>(side)][static_cast<size_t>(move.from())]
                 [static_cast<size_t>(move.to())];
  const int32_t bonus = std::min(depth * depth, MAX_HISTORY);
  h += bonus - (h * bonus / MAX_HISTORY);
}

auto quiescence(Position& pos, int32_t alpha, int32_t beta, int ply,
                SearchContext& ctx) -> int32_t {
  if (shouldStop(ctx)) {
    return 0;
  }
  ++ctx.nodes;
  if (ply >= MAX_PLY) {
    return evaluate(pos);
  }
  const int32_t stand_pat = evaluate(pos);
  if (stand_pat >= beta) {
    return stand_pat;
  }
  alpha = std::max(stand_pat, alpha);
  int32_t best = stand_pat;

  MoveList capture_moves;
  generateCaptureMoves(pos, capture_moves);
  ScoreList scores;
  scoreMoves(pos, capture_moves, scores, Move{}, ctx, ply);

  for (size_t i = 0; i < capture_moves.size(); ++i) {
    pickNext(capture_moves, scores, i);
    const Move move = capture_moves[i];
    const MoveState state = pos.makeMove(move);
    if (inCheck(pos, opposite(pos.sideToMove()))) {
      pos.undoMove(move, state);
      if (isStopped(ctx)) {
        return 0;
      }
      continue;
    }
    const int32_t score = -quiescence(pos, -beta, -alpha, ply + 1, ctx);
    pos.undoMove(move, state);
    if (isStopped(ctx)) {
      return 0;
    }

    best = std::max(score, best);
    alpha = std::max(score, alpha);
    if (score >= beta) {
      break;
    }
  }

  return best;
}

auto negamax(Position& pos, int depth, int32_t alpha, int32_t beta, int ply,
             SearchContext& ctx, bool allow_null) -> int32_t {
  if (shouldStop(ctx)) {
    return 0;
  }
  ++ctx.nodes;
  if (ply >= MAX_PLY) {
    return evaluate(pos);
  }
  if (pos.isRepetition() || pos.isInsufficientMaterial() ||
      pos.halfMoveClock() >= 100) {
    return 0;
  }
  const bool in_check = inCheck(pos, pos.sideToMove());
  if (in_check) {
    ++depth;
  }
  if (depth <= 0) {
    return quiescence(pos, alpha, beta, ply, ctx);
  }

  const uint64_t key{pos.zobristKey()};
  const auto tt_entry{ctx.tt->probe(key)};
  Move tt_move{};
  if (tt_entry.has_value()) {
    tt_move = tt_entry->move;
    const int32_t tt_score = fromTT(tt_entry->score, ply);
    if (tt_entry->depth >= depth &&
        (tt_entry->bound == Bound::EXACT ||
         (tt_entry->bound == Bound::LOWER && tt_score >= beta) ||
         (tt_entry->bound == Bound::UPPER && tt_score <= alpha))) {
      return tt_score;
    }
  }

  const Color us = pos.sideToMove();
  const bool has_pieces = (pos.occupied(us) ^ pos.pieces(us, PieceType::PAWN) ^
                           pos.pieces(us, PieceType::KING)) != 0;
  if (allow_null && !in_check && depth >= 3 && has_pieces &&
      evaluate(pos) >= beta) {
    constexpr int R = 3;
    const MoveState state = pos.makeNullMove();
    const int32_t score =
        -negamax(pos, depth - 1 - R, -beta, -beta + 1, ply + 1, ctx, false);
    pos.undoNullMove(state);
    if (shouldStop(ctx)) {
      return 0;
    }
    if (score >= beta) {
      return score >= MATE_BOUND ? beta : score;
    }
  }

  const int32_t alpha_orig = alpha;
  int32_t best = -INF;
  Move best_move{};
  bool legal_moves_left = false;
  MoveList moves;
  generatePseudoLegalMoves(pos, moves);
  ScoreList scores;
  scoreMoves(pos, moves, scores, tt_move, ctx, ply);

  for (size_t i = 0; i < moves.size(); ++i) {
    pickNext(moves, scores, i);
    const Move move = moves[i];
    const MoveState state = pos.makeMove(move);
    if (inCheck(pos, opposite(pos.sideToMove()))) {
      pos.undoMove(move, state);
      if (isStopped(ctx)) {
        return 0;
      }
      continue;
    }
    legal_moves_left = true;
    const int32_t score =
        -negamax(pos, depth - 1, -beta, -alpha, ply + 1, ctx, true);
    pos.undoMove(move, state);
    if (isStopped(ctx)) {
      return 0;
    }

    if (score > best) {
      best = score;
      best_move = move;
    }
    alpha = std::max(score, alpha);
    if (score >= beta) {
      if (!move.isCapture() && !move.isPromotion()) {
        storeQuietCutoff(ctx, pos.sideToMove(), move, depth, ply);
      }
      break;
    }
  }
  if (!legal_moves_left) {
    best = in_check ? -MATE + ply : 0;
    ctx.tt->store(key, Move{}, toTT(best, ply), depth, Bound::EXACT);
    return best;
  }
  Bound bound{Bound::EXACT};
  if (best >= beta) {
    bound = Bound::LOWER;
  } else if (best <= alpha_orig) {
    bound = Bound::UPPER;
  }
  ctx.tt->store(key, bound == Bound::UPPER ? Move{} : best_move,
                toTT(best, ply), depth, bound);
  return best;
}

auto rootSearch(Position& pos, int depth, Move pv_move, SearchContext& ctx)
    -> SearchResult {
  SearchResult result{.score = -INF, .depth = static_cast<int16_t>(depth)};

  int32_t alpha = -INF;
  MoveList moves;
  bool legal_moves_left = false;
  generatePseudoLegalMoves(pos, moves);
  ScoreList scores;
  scoreMoves(pos, moves, scores, pv_move, ctx, 0);

  for (size_t i = 0; i < moves.size(); ++i) {
    pickNext(moves, scores, i);
    const Move move = moves[i];
    const MoveState state = pos.makeMove(move);
    if (inCheck(pos, opposite(pos.sideToMove()))) {
      pos.undoMove(move, state);
      if (isStopped(ctx)) {
        return result;
      }
      continue;
    }
    legal_moves_left = true;
    const int32_t score = -negamax(pos, depth - 1, -INF, -alpha, 1, ctx, true);
    pos.undoMove(move, state);
    if (isStopped(ctx)) {
      return result;
    }
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

auto search(Position& pos, const SearchLimits& limits, TranspositionTable& tt,
            std::atomic<bool>& stop) -> SearchResult {
  SearchResult result{};
  SearchContext ctx{.limits = limits, .tt = &tt, .stop = &stop};
  for (int d = 1; d <= limits.depth; ++d) {
    const SearchResult iter = rootSearch(pos, d, result.best_move, ctx);
    if (isStopped(ctx)) {
      break;
    }
    result = iter;
    ctx.can_stop = true;
  }
  result.nodes = ctx.nodes;
  return result;
}
