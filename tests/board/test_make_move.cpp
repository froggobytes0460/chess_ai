#include <gtest/gtest.h>

#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move.hpp>
#include <chess_ai/core/move_list.hpp>
#include <chess_ai/core/types.hpp>
#include <chess_ai/movegen/movegen.hpp>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>

namespace {
// One make/undo scenario: `move` played from `fen` must yield `expected`.
struct MakeMoveCase {
  std::string_view name;
  std::string_view fen;
  Move move;
  std::string_view expected;
};

// NOLINTNEXTLINE(readability-identifier-naming) GoogleTest looks up this name.
[[maybe_unused]] auto PrintTo(const MakeMoveCase& param, std::ostream* out)
    -> void {
  *out << param.name << " [" << param.fen << "]";
}
}  // namespace

class MakeMoveTest : public testing::TestWithParam<MakeMoveCase> {};

TEST_P(MakeMoveTest, MakeProducesExpectedPosition) {
  const MakeMoveCase& param = GetParam();
  Position pos(param.fen);
  [[maybe_unused]] const MoveState state = pos.makeMove(param.move);

  EXPECT_EQ(pos.toFEN(), param.expected);
}

// Incrementally updated key must match a fresh position built from the FEN.
TEST_P(MakeMoveTest, MakeKeepsZobristKeyInSync) {
  const MakeMoveCase& param = GetParam();
  Position pos(param.fen);
  [[maybe_unused]] const MoveState state = pos.makeMove(param.move);

  EXPECT_EQ(pos.zobristKey(), pos.computeZobristKey());
  EXPECT_EQ(pos.zobristKey(), Position(param.expected).zobristKey());
}

TEST_P(MakeMoveTest, UndoRestoresOriginalPosition) {
  const MakeMoveCase& param = GetParam();
  Position pos(param.fen);
  const uint64_t key_before = pos.zobristKey();

  const MoveState state = pos.makeMove(param.move);
  pos.undoMove(param.move, state);

  EXPECT_EQ(pos.toFEN(), param.fen);
  EXPECT_EQ(pos.zobristKey(), key_before);
  EXPECT_EQ(pos.zobristKey(), pos.computeZobristKey());
}

using enum Square;
using enum MoveFlag;

INSTANTIATE_TEST_SUITE_P(
    Moves, MakeMoveTest,
    testing::Values(
        // Quiet moves and clocks
        MakeMoveCase{
            .name = "QuietKnightMoveIncrementsHalfMoveClock",
            .fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            .move = {G1, F3, QUIET},
            .expected =
                "rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R b KQkq - 1 1"},
        MakeMoveCase{
            .name = "WhiteDoublePushSetsEnPassantSquare",
            .fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            .move = {E2, E4, DOUBLE_PUSH},
            .expected =
                "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"},
        MakeMoveCase{
            .name = "BlackDoublePushIncrementsFullMoveCounter",
            .fen =
                "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
            .move = {E7, E5, DOUBLE_PUSH},
            .expected = "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w "
                        "KQkq e6 0 2"},
        MakeMoveCase{
            .name = "NonPushClearsEnPassantSquare",
            .fen =
                "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
            .move = {G8, F6, QUIET},
            .expected =
                "rnbqkb1r/pppppppp/5n2/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 1 2"},

        // Captures
        MakeMoveCase{.name = "CaptureResetsHalfMoveClock",
                     .fen = "4k3/8/8/3p4/4P3/8/8/4K3 w - - 5 10",
                     .move = {E4, D5, CAPTURE},
                     .expected = "4k3/8/8/3P4/8/8/8/4K3 b - - 0 10"},
        MakeMoveCase{.name = "NonPawnCaptureResetsHalfMoveClock",
                     .fen = "4k3/8/8/3p4/8/4N3/8/4K3 w - - 7 10",
                     .move = {E3, D5, CAPTURE},
                     .expected = "4k3/8/8/3N4/8/8/8/4K3 b - - 0 10"},
        MakeMoveCase{.name = "WhiteEnPassantRemovesCapturedPawn",
                     .fen = "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2",
                     .move = {E5, D6, EN_PASSANT},
                     .expected = "4k3/8/3P4/8/8/8/8/4K3 b - - 0 2"},
        MakeMoveCase{.name = "BlackEnPassantRemovesCapturedPawn",
                     .fen = "4k3/8/8/8/3Pp3/8/8/4K3 b - d3 0 2",
                     .move = {E4, D3, EN_PASSANT},
                     .expected = "4k3/8/8/8/8/3p4/8/4K3 w - - 0 3"},

        // Promotions
        MakeMoveCase{.name = "WhitePromotesToQueen",
                     .fen = "4k3/P7/8/8/8/8/8/4K3 w - - 0 1",
                     .move = {A7, A8, PROMOTION_QUEEN},
                     .expected = "Q3k3/8/8/8/8/8/8/4K3 b - - 0 1"},
        MakeMoveCase{.name = "BlackPromotesToRook",
                     .fen = "4k3/8/8/8/8/8/7p/4K3 b - - 3 9",
                     .move = {H2, H1, PROMOTION_ROOK},
                     .expected = "4k3/8/8/8/8/8/8/4K2r w - - 0 10"},
        MakeMoveCase{.name = "CapturePromotionToKnight",
                     .fen = "1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1",
                     .move = {A7, B8, PROMOTION_CAPTURE_KNIGHT},
                     .expected = "1N2k3/8/8/8/8/8/8/4K3 b - - 0 1"},

        // Castling
        MakeMoveCase{.name = "WhiteCastlesKingside",
                     .fen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
                     .move = {E1, G1, CASTLE_KINGSIDE},
                     .expected = "r3k2r/8/8/8/8/8/8/R4RK1 b kq - 1 1"},
        MakeMoveCase{.name = "WhiteCastlesQueenside",
                     .fen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
                     .move = {E1, C1, CASTLE_QUEENSIDE},
                     .expected = "r3k2r/8/8/8/8/8/8/2KR3R b kq - 1 1"},
        MakeMoveCase{.name = "BlackCastlesKingside",
                     .fen = "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1",
                     .move = {E8, G8, CASTLE_KINGSIDE},
                     .expected = "r4rk1/8/8/8/8/8/8/R3K2R w KQ - 1 2"},
        MakeMoveCase{.name = "BlackCastlesQueenside",
                     .fen = "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1",
                     .move = {E8, C8, CASTLE_QUEENSIDE},
                     .expected = "2kr3r/8/8/8/8/8/8/R3K2R w KQ - 1 2"},

        // Castling-rights updates
        MakeMoveCase{.name = "KingMoveClearsBothOwnRights",
                     .fen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
                     .move = {E1, E2, QUIET},
                     .expected = "r3k2r/8/8/8/8/8/4K3/R6R b kq - 1 1"},
        MakeMoveCase{.name = "KingsideRookMoveClearsOnlyKingsideRight",
                     .fen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
                     .move = {H1, H2, QUIET},
                     .expected = "r3k2r/8/8/8/8/8/7R/R3K3 b Qkq - 1 1"},
        // Moving off A1 drops Q; capturing on A8 drops q.
        MakeMoveCase{.name = "RookCapturesRookClearsBothSidesRights",
                     .fen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
                     .move = {A1, A8, CAPTURE},
                     .expected = "R3k2r/8/8/8/8/8/8/4K2R b Kk - 0 1"}),
    [](const testing::TestParamInfo<MakeMoveCase>& info) -> std::string {
      return std::string(info.param.name);
    });

// Every pseudo-legal move in a varied position must round-trip through
// make/undo, and keep the incremental Zobrist key in sync after make.
class MakeUndoRoundTripTest : public testing::TestWithParam<std::string_view> {
};

TEST_P(MakeUndoRoundTripTest, AllPseudoLegalMovesRoundTrip) {
  const std::string fen(GetParam());
  Position pos(fen);
  const uint64_t key_before = pos.zobristKey();

  MoveList moves;
  generatePseudoLegalMoves(pos, moves);
  ASSERT_GT(moves.size(), 0U);

  for (const Move move : moves) {
    const MoveState state = pos.makeMove(move);
    EXPECT_EQ(pos.zobristKey(), pos.computeZobristKey())
        << "key mismatch after " << static_cast<int>(move.from()) << "->"
        << static_cast<int>(move.to());

    pos.undoMove(move, state);
    EXPECT_EQ(pos.toFEN(), fen)
        << "undo failed for " << static_cast<int>(move.from()) << "->"
        << static_cast<int>(move.to());
    EXPECT_EQ(pos.zobristKey(), key_before);
  }
}

INSTANTIATE_TEST_SUITE_P(
    Positions, MakeUndoRoundTripTest,
    testing::Values(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        // Kiwipete: castling both sides, many captures.
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R b KQkq - 0 1",
        // Perft position 5: promotions and promotion-captures.
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        // En passant available.
        "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3"));
