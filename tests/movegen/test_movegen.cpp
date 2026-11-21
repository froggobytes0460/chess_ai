#include <gtest/gtest.h>

#include <algorithm>
#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move.hpp>
#include <chess_ai/core/move_list.hpp>
#include <chess_ai/core/types.hpp>
#include <chess_ai/movegen/movegen.hpp>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace {
// One movegen scenario. `from == Square::NONE` makes `count`/`captures` apply
// to the whole move list instead of moves from a single square.
struct MoveGenCase {
  std::string_view name;
  std::string_view fen;
  Square from = Square::NONE;
  std::optional<int> count = std::nullopt;
  std::optional<int> captures = std::nullopt;
  // `= {}` keeps GCC -Wmissing-field-initializers quiet on partial init.
  std::vector<Move> present = {};  // NOLINT(readability-redundant-member-init)
  std::vector<Move> absent = {};   // NOLINT(readability-redundant-member-init)
};

// NOLINTNEXTLINE(readability-identifier-naming) GoogleTest looks up this name.
[[maybe_unused]] auto PrintTo(const MoveGenCase& param, std::ostream* out)
    -> void {
  *out << param.name << " [" << param.fen << ']';
}

auto contains(const MoveList& moves, const Move target) -> bool {
  return std::ranges::any_of(
      moves, [&](const Move move) -> bool { return move == target; });
}

auto countMatching(const MoveList& moves, Square from, bool captures_only)
    -> int {
  return static_cast<int>(
      std::ranges::count_if(moves, [&](const Move move) -> bool {
        return (from == Square::NONE || move.from() == from) &&
               (!captures_only || move.isCapture());
      }));
}
}  // namespace

class MoveGenTest : public testing::TestWithParam<MoveGenCase> {};

TEST_P(MoveGenTest, GeneratesExpectedMoves) {
  const MoveGenCase& param = GetParam();
  Position pos(param.fen);
  MoveList moves;
  generatePseudoLegalMoves(pos, moves);

  if (param.count) {
    EXPECT_EQ(countMatching(moves, param.from, false), *param.count);
  }
  if (param.captures) {
    EXPECT_EQ(countMatching(moves, param.from, true), *param.captures);
  }
  for (const Move move : param.present) {
    EXPECT_TRUE(contains(moves, move))
        << "missing " << static_cast<int>(move.from()) << "->"
        << static_cast<int>(move.to());
  }
  for (const Move move : param.absent) {
    EXPECT_FALSE(contains(moves, move))
        << "unexpected " << static_cast<int>(move.from()) << "->"
        << static_cast<int>(move.to());
  }
}

using enum Square;
using enum MoveFlag;

INSTANTIATE_TEST_SUITE_P(
    PseudoLegal, MoveGenTest,
    testing::Values(
        // Pawns
        MoveGenCase{.name = "WhiteSinglePushOnOpenBoard",
                    .fen = "8/8/8/8/8/4P3/8/8 w - - 0 1",
                    .from = E3,
                    .count = 1,
                    .present = {{E3, E4, QUIET}}},
        MoveGenCase{.name = "WhiteDoublePushFromStartRank",
                    .fen = "8/8/8/8/8/8/4P3/8 w - - 0 1",
                    .from = E2,
                    .count = 2,
                    .present = {{E2, E3, QUIET}, {E2, E4, DOUBLE_PUSH}}},
        MoveGenCase{.name = "BlackDoublePushFromStartRank",
                    .fen = "8/4p3/8/8/8/8/8/8 b - - 0 1",
                    .from = E7,
                    .count = 2,
                    .present = {{E7, E6, QUIET}, {E7, E5, DOUBLE_PUSH}}},
        MoveGenCase{.name = "PushBlockedByOccupiedSquareGeneratesNothing",
                    .fen = "8/8/8/8/4p3/4P3/8/8 w - - 0 1",
                    .from = E3,
                    .count = 0},
        MoveGenCase{.name = "DoublePushBlockedTwoAheadStillAllowsSinglePush",
                    .fen = "8/8/8/8/4p3/8/4P3/8 w - - 0 1",
                    .from = E2,
                    .count = 1,
                    .present = {{E2, E3, QUIET}},
                    .absent = {{E2, E4, DOUBLE_PUSH}}},
        MoveGenCase{.name = "WhitePushPromotesOnRank8",
                    .fen = "8/4P3/8/8/8/8/8/8 w - - 0 1",
                    .from = E7,
                    .count = 4,
                    .present = {{E7, E8, PROMOTION_KNIGHT},
                                {E7, E8, PROMOTION_BISHOP},
                                {E7, E8, PROMOTION_ROOK},
                                {E7, E8, PROMOTION_QUEEN}}},
        MoveGenCase{.name = "BlackPushPromotesOnRank1",
                    .fen = "8/8/8/8/8/8/4p3/8 b - - 0 1",
                    .from = E2,
                    .count = 4,
                    .present = {{E2, E1, PROMOTION_QUEEN}}},
        MoveGenCase{.name = "PawnCapturesBothDiagonals",
                    .fen = "8/8/8/3pppp1/4P3/8/8/8 w - - 0 1",
                    .from = E4,
                    .count = 2,
                    .present = {{E4, D5, CAPTURE}, {E4, F5, CAPTURE}}},
        // Enemy pawn on H3: if file-wraparound existed in the attack table,
        // A2's "left" capture target would incorrectly land on H3.
        MoveGenCase{.name = "AFilePawnHasNoLeftCaptureNoWraparound",
                    .fen = "8/8/8/8/8/7p/P7/8 w - - 0 1",
                    .from = A2,
                    .captures = 0},
        MoveGenCase{.name = "HFilePawnCapturesOnlyLeftDiagonal",
                    .fen = "8/8/8/7p/6PP/8/8/8 b - - 0 1",
                    .from = H5,
                    .count = 1,
                    .present = {{H5, G4, CAPTURE}}},
        MoveGenCase{.name = "CapturePromotesOnBackRank",
                    .fen = "3n4/4P3/8/8/8/8/8/8 w - - 0 1",
                    .from = E7,
                    .count = 8,
                    .present = {{E7, D8, PROMOTION_CAPTURE_QUEEN},
                                {E7, E8, PROMOTION_QUEEN}}},
        MoveGenCase{.name = "EnPassantGeneratedWhenEpSquareSet",
                    .fen = "8/8/8/3pP3/8/8/8/8 w - d6 0 1",
                    .from = E5,
                    .present = {{E5, D6, EN_PASSANT}}},
        MoveGenCase{.name = "NoEnPassantWhenEpSquareNotSet",
                    .fen = "8/8/8/3pP3/8/8/8/8 w - - 0 1",
                    .from = E5,
                    .absent = {{E5, D6, EN_PASSANT}}},

        // Knights
        MoveGenCase{.name = "CentralKnightHasEightMoves",
                    .fen = "8/8/8/8/3N4/8/8/8 w - - 0 1",
                    .from = D4,
                    .count = 8,
                    .present = {{D4, B3, QUIET}, {D4, F5, QUIET}}},
        MoveGenCase{.name = "CornerKnightHasTwoMovesNoWraparound",
                    .fen = "8/8/8/8/8/8/8/N7 w - - 0 1",
                    .count = 2,
                    .present = {{A1, B3, QUIET}, {A1, C2, QUIET}}},
        MoveGenCase{.name = "KnightOwnPiecesBlockTargets",
                    .fen = "8/8/2P1P3/8/3N4/8/8/8 w - - 0 1",
                    .from = D4,
                    .count = 6,
                    .absent = {{D4, C6, QUIET}, {D4, E6, CAPTURE}}},
        MoveGenCase{.name = "KnightEnemyPieceGivesCapture",
                    .fen = "8/8/8/1p6/3N4/8/8/8 w - - 0 1",
                    .from = D4,
                    .count = 8,
                    .captures = 1,
                    .present = {{D4, B5, CAPTURE}}},

        // Sliders
        MoveGenCase{.name = "RookOnEmptyBoardHasFourteenMoves",
                    .fen = "8/8/8/8/3R4/8/8/8 w - - 0 1",
                    .from = D4,
                    .count = 14},
        MoveGenCase{.name = "BishopOnEmptyBoardHasThirteenMoves",
                    .fen = "8/8/8/8/3B4/8/8/8 w - - 0 1",
                    .from = D4,
                    .count = 13},
        MoveGenCase{.name = "QueenOnEmptyBoardHasTwentySevenMoves",
                    .fen = "8/8/8/8/3Q4/8/8/8 w - - 0 1",
                    .from = D4,
                    .count = 27},
        MoveGenCase{.name = "RookStopsBeforeOwnBlocker",
                    .fen = "8/8/8/8/8/P7/8/R7 w - - 0 1",
                    .from = A1,
                    .count = 8,
                    .present = {{A1, A2, QUIET}},
                    .absent = {{A1, A3, QUIET}}},
        MoveGenCase{.name = "RookCapturesEnemyBlockerAndStops",
                    .fen = "8/8/8/8/8/p7/8/R7 w - - 0 1",
                    .from = A1,
                    .count = 9,
                    .present = {{A1, A3, CAPTURE}},
                    .absent = {{A1, A4, QUIET}}},
        MoveGenCase{.name = "BishopCapturesEnemyBlockerAndStops",
                    .fen = "8/8/8/8/8/2p5/8/B7 w - - 0 1",
                    .from = A1,
                    .count = 2,
                    .present = {{A1, B2, QUIET}, {A1, C3, CAPTURE}}},

        // Kings
        MoveGenCase{.name = "CentralKingHasEightMoves",
                    .fen = "8/8/8/8/3K4/8/8/8 w - - 0 1",
                    .from = D4,
                    .count = 8},
        MoveGenCase{.name = "CornerKingHasThreeMoves",
                    .fen = "8/8/8/8/8/8/8/K7 w - - 0 1",
                    .count = 3},
        // A2 own pawn blocks; B1, B2 free. Pawn A2 blocked by a3.
        MoveGenCase{.name = "KingOwnBlockAndEnemyCapture",
                    .fen = "8/8/8/8/8/p7/P7/K7 w - - 0 1",
                    .from = A1,
                    .count = 2,
                    .captures = 0},

        // Castling
        MoveGenCase{
            .name = "WhiteCastlesBothSides",
            .fen = "8/8/8/8/8/8/8/R3K2R w KQ - 0 1",
            .from = E1,
            .count = 7,
            .present = {{E1, G1, CASTLE_KINGSIDE}, {E1, C1, CASTLE_QUEENSIDE}}},
        MoveGenCase{
            .name = "BlackCastlesBothSides",
            .fen = "r3k2r/8/8/8/8/8/8/8 b kq - 0 1",
            .from = E8,
            .count = 7,
            .present = {{E8, G8, CASTLE_KINGSIDE}, {E8, C8, CASTLE_QUEENSIDE}}},
        MoveGenCase{
            .name = "NoCastlingWithoutRights",
            .fen = "8/8/8/8/8/8/8/R3K2R w - - 0 1",
            .from = E1,
            .count = 5,
            .absent = {{E1, G1, CASTLE_KINGSIDE}, {E1, C1, CASTLE_QUEENSIDE}}},
        MoveGenCase{.name = "OnlyKingsideRightGivesOnlyKingsideCastle",
                    .fen = "8/8/8/8/8/8/8/R3K2R w K - 0 1",
                    .from = E1,
                    .count = 6,
                    .present = {{E1, G1, CASTLE_KINGSIDE}},
                    .absent = {{E1, C1, CASTLE_QUEENSIDE}}},
        // B1 is not crossed by the king, but the rook must pass it.
        MoveGenCase{.name = "QueensideBlockedByPieceOnB1",
                    .fen = "8/8/8/8/8/8/8/RN2K2R w KQ - 0 1",
                    .present = {{E1, G1, CASTLE_KINGSIDE}},
                    .absent = {{E1, C1, CASTLE_QUEENSIDE}}},
        MoveGenCase{.name = "KingsideBlockedByPieceOnF1",
                    .fen = "8/8/8/8/8/8/8/R3KB1R w KQ - 0 1",
                    .present = {{E1, C1, CASTLE_QUEENSIDE}},
                    .absent = {{E1, G1, CASTLE_KINGSIDE}}},
        MoveGenCase{
            .name = "NoCastlingOutOfCheck",
            .fen = "4r3/8/8/8/8/8/8/R3K2R w KQ - 0 1",
            .absent = {{E1, G1, CASTLE_KINGSIDE}, {E1, C1, CASTLE_QUEENSIDE}}},
        MoveGenCase{.name = "NoKingsideCastleThroughAttackedF1",
                    .fen = "5r2/8/8/8/8/8/8/R3K2R w KQ - 0 1",
                    .present = {{E1, C1, CASTLE_QUEENSIDE}},
                    .absent = {{E1, G1, CASTLE_KINGSIDE}}},
        MoveGenCase{.name = "NoQueensideCastleThroughAttackedD1",
                    .fen = "3r4/8/8/8/8/8/8/R3K2R w KQ - 0 1",
                    .present = {{E1, G1, CASTLE_KINGSIDE}},
                    .absent = {{E1, C1, CASTLE_QUEENSIDE}}},
        // King never crosses B1, so an attack there does not stop castling.
        MoveGenCase{
            .name = "QueensideCastleAllowedWhenOnlyB1Attacked",
            .fen = "1r6/8/8/8/8/8/8/R3K2R w KQ - 0 1",
            .present = {{E1, G1, CASTLE_KINGSIDE}, {E1, C1, CASTLE_QUEENSIDE}}},

        // Full positions
        MoveGenCase{
            .name = "StartPositionWhiteHasTwentyMoves",
            .fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            .count = 20},
        MoveGenCase{
            .name = "StartPositionBlackHasTwentyMoves",
            .fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1",
            .count = 20},
        // 48 legal moves incl. 2 castles. No pinned or king-exposing moves
        // exist here, so pseudo-legal count equals it.
        MoveGenCase{
            .name = "KiwipeteMoveCount",
            .fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/"
                   "R3K2R w KQkq - 0 1",
            .count = 48,
            .present = {{E1, G1, CASTLE_KINGSIDE}, {E1, C1, CASTLE_QUEENSIDE}}},
        // 44 legal incl. 1 castle.
        MoveGenCase{.name = "PerftPosition5MoveCount",
                    .fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w "
                           "KQ - 1 8",
                    .count = 44,
                    .present = {{E1, G1, CASTLE_KINGSIDE}}}),
    [](const testing::TestParamInfo<MoveGenCase>& info) -> std::string {
      return std::string(info.param.name);
    });
