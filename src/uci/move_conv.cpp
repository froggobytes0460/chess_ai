#include <chess_ai/core/move_list.hpp>
#include <chess_ai/core/types.hpp>
#include <chess_ai/movegen/movegen.hpp>
#include <chess_ai/uci/move_conv.hpp>
#include <optional>
#include <string>
#include <string_view>

namespace {
constexpr int BOARD_WIDTH{8};
constexpr std::string_view PIECE_CHARS{"kqrbnp"};

void appendSquare(std::string& out, Square sq) {
  const auto idx{static_cast<unsigned>(sq)};
  out += static_cast<char>('a' + (idx % BOARD_WIDTH));
  out += static_cast<char>('1' + (idx / BOARD_WIDTH));
}
}  // namespace

auto moveToUci(Move move) -> std::string {
  std::string out;
  appendSquare(out, move.from());
  appendSquare(out, move.to());
  if (move.isPromotion()) {
    out += PIECE_CHARS[static_cast<size_t>(move.promotionType())];
  }
  return out;
}

auto uciToMove(const Position& pos, std::string_view uci)
    -> std::optional<Move> {
  MoveList moves;
  generatePseudoLegalMoves(pos, moves);

  for (const Move move : moves) {
    if (moveToUci(move) == uci) {
      return move;
    }
  }
  return std::nullopt;
}
