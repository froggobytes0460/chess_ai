#include <chess_ai/move_list.hpp>
#include <chess_ai/movegen.hpp>
#include <chess_ai/perft.hpp>
#include <cstdint>
#include <optional>

namespace {

// Node count below `move`, or nullopt if `move` leaves the mover in check.
auto legalSubtreeNodes(Position& position, Move move, int depth)
    -> std::optional<uint64_t> {
  const MoveState state = position.makeMove(move);
  std::optional<uint64_t> nodes;
  if (!inCheck(position, opposite(position.sideToMove()))) {
    nodes = perft(position, depth - 1);
  }
  position.undoMove(move, state);
  return nodes;
}

}  // namespace

auto perft(Position& position, int depth) -> uint64_t {
  if (depth <= 0) {
    return 1;
  }

  MoveList moves;
  generatePseudoLegalMoves(position, moves);

  uint64_t total{0};
  for (const Move move : moves) {
    total += legalSubtreeNodes(position, move, depth).value_or(0);
  }
  return total;
}

auto divide(Position& position, int depth)
    -> std::vector<std::pair<Move, uint64_t>> {
  MoveList moves;
  generatePseudoLegalMoves(position, moves);

  std::vector<std::pair<Move, uint64_t>> result;
  for (const Move move : moves) {
    if (const auto nodes = legalSubtreeNodes(position, move, depth)) {
      result.emplace_back(move, *nodes);
    }
  }
  return result;
}
