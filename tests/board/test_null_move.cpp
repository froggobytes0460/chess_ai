#include <gtest/gtest.h>

#include <chess_ai/board/position.hpp>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>

namespace {
// One null-move scenario: passing from `fen` must yield `expected`.
struct NullMoveCase {
  std::string_view name;
  std::string_view fen;
  std::string_view expected;
};

// NOLINTNEXTLINE(readability-identifier-naming) GoogleTest looks up this name.
[[maybe_unused]] auto PrintTo(const NullMoveCase& param, std::ostream* out)
    -> void {
  *out << param.name << " [" << param.fen << ']';
}
}  // namespace

class NullMoveTest : public testing::TestWithParam<NullMoveCase> {};

// Null move flips side to move, clears en passant, resets the half-move clock
// and leaves pieces, castling rights and the full-move counter untouched.
TEST_P(NullMoveTest, MakeProducesExpectedPosition) {
  const NullMoveCase& param = GetParam();
  Position pos(param.fen);
  static_cast<void>(pos.makeNullMove());

  EXPECT_EQ(pos.toFEN(), param.expected);
}

// Incrementally updated key must match a fresh position built from the FEN.
TEST_P(NullMoveTest, MakeKeepsZobristKeyInSync) {
  const NullMoveCase& param = GetParam();
  Position pos(param.fen);
  static_cast<void>(pos.makeNullMove());

  EXPECT_EQ(pos.zobristKey(), pos.computeZobristKey());
  EXPECT_EQ(pos.zobristKey(), Position(param.expected).zobristKey());
}

TEST_P(NullMoveTest, UndoRestoresOriginalPosition) {
  const NullMoveCase& param = GetParam();
  Position pos(param.fen);
  const uint64_t key_before{pos.zobristKey()};

  const MoveState state = pos.makeNullMove();
  pos.undoNullMove(state);

  EXPECT_EQ(pos.toFEN(), param.fen);
  EXPECT_EQ(pos.zobristKey(), key_before);
  EXPECT_EQ(pos.zobristKey(), pos.computeZobristKey());
}

// Reset half-move clock means no earlier position can count as a repetition.
TEST_P(NullMoveTest, MakeIsNotRepetition) {
  const NullMoveCase& param = GetParam();
  Position pos(param.fen);
  static_cast<void>(pos.makeNullMove());

  EXPECT_FALSE(pos.isRepetition());
}

INSTANTIATE_TEST_SUITE_P(
    Positions, NullMoveTest,
    testing::Values(
        NullMoveCase{
            .name = "StartPositionWhiteToMove",
            .fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            .expected =
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1"},
        NullMoveCase{
            .name = "BlackToMoveClearsEnPassantSquare",
            .fen =
                "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
            .expected =
                "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1"},
        NullMoveCase{
            .name = "WhiteToMoveClearsCapturableEnPassantSquare",
            .fen =
                "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3",
            .expected =
                "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 3"},
        // Kiwipete with non-zero clock: castling rights kept, clock reset.
        NullMoveCase{
            .name = "KiwipeteResetsHalfMoveClock",
            .fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w "
                   "KQkq - 7 12",
            .expected = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/"
                        "R3K2R b KQkq - 0 12"}),
    [](const testing::TestParamInfo<NullMoveCase>& info) -> std::string {
      return std::string(info.param.name);
    });
