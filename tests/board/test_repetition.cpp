#include <gtest/gtest.h>

#include <array>
#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move.hpp>
#include <chess_ai/core/types.hpp>
#include <cstddef>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr std::string_view STARTPOS{
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"};

// Playing `moves` from `fen` must leave isRepetition() == `expected`.
struct RepetitionCase {
  std::string_view name;
  std::string_view fen;
  std::vector<Move> moves;
  bool expected;
};

// NOLINTNEXTLINE(readability-identifier-naming) GoogleTest looks up this name.
[[maybe_unused]] auto PrintTo(const RepetitionCase& param, std::ostream* out)
    -> void {
  *out << param.name << " [" << param.fen << ']';
}

auto playAll(Position& pos, std::span<const Move> moves)
    -> std::vector<MoveState> {
  std::vector<MoveState> states;
  states.reserve(moves.size());
  for (const Move move : moves) {
    states.push_back(pos.makeMove(move));
  }
  return states;
}

using enum Square;
using enum MoveFlag;

constexpr std::array<Move, 4> KNIGHT_SHUFFLE{
    Move{G1, F3, QUIET},
    Move{G8, F6, QUIET},
    Move{F3, G1, QUIET},
    Move{F6, G8, QUIET},
};
}  // namespace

class RepetitionTest : public testing::TestWithParam<RepetitionCase> {};

TEST_P(RepetitionTest, DetectsRepetitionAfterMoves) {
  const RepetitionCase& param = GetParam();
  Position pos{param.fen};
  playAll(pos, param.moves);

  EXPECT_EQ(pos.isRepetition(), param.expected);
}

INSTANTIATE_TEST_SUITE_P(
    Sequences, RepetitionTest,
    testing::Values(
        RepetitionCase{.name = "FreshPositionDoesNotRepeat",
                       .fen = STARTPOS,
                       .moves = {},
                       .expected = false},
        // Clock larger than stored history must not read before index 0.
        RepetitionCase{
            .name = "HalfMoveClockFromFenExceedsHistory",
            .fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 10 6",
            .moves = {{G1, F3, QUIET}},
            .expected = false},
        RepetitionCase{.name = "KnightShuffleRepeatsStartpos",
                       .fen = STARTPOS,
                       .moves = {KNIGHT_SHUFFLE.begin(), KNIGHT_SHUFFLE.end()},
                       .expected = true},
        RepetitionCase{
            .name = "IncompleteShuffleDoesNotRepeat",
            .fen = STARTPOS,
            .moves = {KNIGHT_SHUFFLE.begin(), KNIGHT_SHUFFLE.end() - 1},
            .expected = false},
        // Same squares but castling rights lost: different position.
        RepetitionCase{.name = "LostCastlingRightsDoNotRepeat",
                       .fen = "4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1",
                       .moves = {{E1, F1, QUIET},
                                 {E8, D8, QUIET},
                                 {F1, E1, QUIET},
                                 {D8, E8, QUIET}},
                       .expected = false},
        // Clock resets on the capture; the window must still reach back to
        // the position right after it (off-by-one guard).
        RepetitionCase{.name = "ShuffleAfterCaptureRepeats",
                       .fen = "4k3/8/8/p7/8/8/8/R3K3 w - - 0 1",
                       .moves = {{A1, A5, CAPTURE},
                                 {E8, D8, QUIET},
                                 {E1, D1, QUIET},
                                 {D8, E8, QUIET},
                                 {D1, E1, QUIET}},
                       .expected = true}),
    [](const testing::TestParamInfo<RepetitionCase>& info) -> std::string {
      return std::string(info.param.name);
    });

// Undo must pop key history: after unwinding the shuffle nothing repeats,
// and replaying it detects the repetition again.
TEST(RepetitionUndoTest, UndoPopsKeyHistory) {
  Position pos{STARTPOS};
  std::vector<MoveState> states = playAll(pos, KNIGHT_SHUFFLE);
  ASSERT_TRUE(pos.isRepetition());

  for (size_t i = KNIGHT_SHUFFLE.size(); i-- > 0;) {
    pos.undoMove(KNIGHT_SHUFFLE[i], states[i]);
    EXPECT_FALSE(pos.isRepetition()) << "after undoing move " << i;
  }

  states = playAll(pos, KNIGHT_SHUFFLE);
  EXPECT_TRUE(pos.isRepetition());
}
