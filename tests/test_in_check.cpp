#include <gtest/gtest.h>

#include <chess_ai/movegen.hpp>
#include <chess_ai/position.hpp>
#include <chess_ai/types.hpp>
#include <ostream>
#include <string>
#include <string_view>

namespace {
struct InCheckCase {
  std::string_view name;
  std::string_view fen;
  Color color;
  bool expected;
};

// NOLINTNEXTLINE(readability-identifier-naming) GoogleTest looks up this name.
[[maybe_unused]] auto PrintTo(const InCheckCase& param, std::ostream* out)
    -> void {
  *out << param.name << " [" << param.fen << "]";
}
}  // namespace

class InCheckTest : public testing::TestWithParam<InCheckCase> {};

TEST_P(InCheckTest, DetectsCheck) {
  const InCheckCase& param = GetParam();
  const Position pos(param.fen);
  EXPECT_EQ(inCheck(pos, param.color), param.expected);
}

using enum Color;

INSTANTIATE_TEST_SUITE_P(
    Positions, InCheckTest,
    testing::Values(
        InCheckCase{
            .name = "StartPositionWhiteNotInCheck",
            .fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            .color = WHITE,
            .expected = false},
        InCheckCase{
            .name = "StartPositionBlackNotInCheck",
            .fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            .color = BLACK,
            .expected = false},

        // Sliders
        InCheckCase{.name = "RookOnRankGivesCheck",
                    .fen = "4k3/8/8/8/8/8/8/4K2r w - - 0 1",
                    .color = WHITE,
                    .expected = true},
        // Same position: the attacker's own king is not in check.
        InCheckCase{.name = "CheckingSideIsNotInCheck",
                    .fen = "4k3/8/8/8/8/8/8/4K2r w - - 0 1",
                    .color = BLACK,
                    .expected = false},
        InCheckCase{.name = "RookBlockedByPieceGivesNoCheck",
                    .fen = "4k3/8/8/8/8/8/8/4KN1r w - - 0 1",
                    .color = WHITE,
                    .expected = false},
        InCheckCase{.name = "BishopOnDiagonalGivesCheck",
                    .fen = "4k3/8/8/b7/8/8/8/4K3 w - - 0 1",
                    .color = WHITE,
                    .expected = true},
        InCheckCase{.name = "BishopBlockedByPieceGivesNoCheck",
                    .fen = "4k3/8/8/b7/8/2P5/8/4K3 w - - 0 1",
                    .color = WHITE,
                    .expected = false},
        InCheckCase{.name = "QueenOnFileChecksBlackKing",
                    .fen = "4k3/8/8/8/8/8/8/4QK2 b - - 0 1",
                    .color = BLACK,
                    .expected = true},

        // Knights
        InCheckCase{.name = "KnightGivesCheck",
                    .fen = "4k3/8/8/8/8/3n4/8/4K3 w - - 0 1",
                    .color = WHITE,
                    .expected = true},
        InCheckCase{.name = "KnightNotOnAttackSquareGivesNoCheck",
                    .fen = "4k3/8/8/8/8/4n3/8/4K3 w - - 0 1",
                    .color = WHITE,
                    .expected = false},

        // Pawns
        InCheckCase{.name = "BlackPawnChecksWhiteKingDiagonally",
                    .fen = "4k3/8/8/8/8/8/3p4/4K3 w - - 0 1",
                    .color = WHITE,
                    .expected = true},
        InCheckCase{.name = "WhitePawnChecksBlackKingDiagonally",
                    .fen = "4k3/3P4/8/8/8/8/8/4K3 b - - 0 1",
                    .color = BLACK,
                    .expected = true},
        InCheckCase{.name = "PawnDirectlyInFrontGivesNoCheck",
                    .fen = "4k3/8/8/8/8/8/4p3/4K3 w - - 0 1",
                    .color = WHITE,
                    .expected = false},
        // Black pawn on D3 attacks C2/E2, never E4 behind it.
        InCheckCase{.name = "PawnDoesNotAttackBackward",
                    .fen = "4k3/8/8/8/4K3/3p4/8/8 w - - 0 1",
                    .color = WHITE,
                    .expected = false}),
    [](const testing::TestParamInfo<InCheckCase>& info) -> std::string {
      return std::string(info.param.name);
    });
