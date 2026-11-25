#include <gtest/gtest.h>

#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move.hpp>
#include <chess_ai/core/move_list.hpp>
#include <chess_ai/core/types.hpp>
#include <chess_ai/movegen/movegen.hpp>
#include <chess_ai/uci/move_conv.hpp>
#include <optional>
#include <ostream>
#include <string>

namespace {
constexpr const char* STARTPOS{
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
};
constexpr const char* KIWIPETE{
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
};
constexpr const char* CASTLING_WHITE{
    "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
};
constexpr const char* CASTLING_BLACK{
    "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1",
};
// White pawn on e7 can push to e8 or capture the rook on d8.
constexpr const char* PROMOTION_WHITE{
    "3r3k/4P3/8/8/8/8/8/4K3 w - - 0 1",
};
constexpr const char* PROMOTION_BLACK{
    "4k3/8/8/8/8/8/4p3/K7 b - - 0 1",
};

struct ConvCase {
  const char* name{};
  const char* fen{};
  const char* uci{};
  Move move{};  // NOLINT(readability-redundant-member-init): GCC
                // -Wmissing-field-initializers on designated init
};

// NOLINTNEXTLINE(readability-identifier-naming) Google GTest uses this name
[[maybe_unused]] void PrintTo(const ConvCase& param, std::ostream* out) {
  *out << param.name << '[' << param.fen << "] " << param.uci;
}

struct RejectCase {
  const char* name{};
  const char* fen{};
  const char* uci{};
};

// NOLINTNEXTLINE(readability-identifier-naming) Google GTest uses this name
[[maybe_unused]] void PrintTo(const RejectCase& param, std::ostream* out) {
  *out << param.name << '[' << param.fen << "] '" << param.uci << '\'';
}

struct FenCase {
  const char* name{};
  const char* fen{};
};

// NOLINTNEXTLINE(readability-identifier-naming) Google GTest uses this name
[[maybe_unused]] void PrintTo(const FenCase& param, std::ostream* out) {
  *out << param.name << '[' << param.fen << ']';
}
}  // namespace

class MoveConvTest : public testing::TestWithParam<ConvCase> {};

TEST_P(MoveConvTest, MoveToUci) {
  const ConvCase& param = GetParam();
  EXPECT_EQ(moveToUci(param.move), param.uci);
}

TEST_P(MoveConvTest, UciToMove) {
  const ConvCase& param = GetParam();
  const Position pos(param.fen);
  const std::optional<Move> move = uciToMove(pos, param.uci);
  ASSERT_TRUE(move.has_value());
  if (move.has_value()) {
    EXPECT_EQ(*move, param.move);
  }
}

INSTANTIATE_TEST_SUITE_P(
    Conversions, MoveConvTest,
    testing::Values(
        ConvCase{
            .name = "Quiet",
            .fen = STARTPOS,
            .uci = "g1f3",
            .move = Move(Square::G1, Square::F3, MoveFlag::QUIET),
        },
        ConvCase{
            .name = "DoublePush",
            .fen = STARTPOS,
            .uci = "e2e4",
            .move = Move(Square::E2, Square::E4, MoveFlag::DOUBLE_PUSH),
        },
        ConvCase{
            .name = "Capture",
            .fen = "4k3/8/8/3p4/4P3/8/8/4K3 w - - 0 1",
            .uci = "e4d5",
            .move = Move(Square::E4, Square::D5, MoveFlag::CAPTURE),
        },
        ConvCase{
            .name = "EnPassant",
            .fen = "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1",
            .uci = "e5d6",
            .move = Move(Square::E5, Square::D6, MoveFlag::EN_PASSANT),
        },
        ConvCase{
            .name = "WhiteKingside",
            .fen = CASTLING_WHITE,
            .uci = "e1g1",
            .move = Move(Square::E1, Square::G1, MoveFlag::CASTLE_KINGSIDE),
        },
        ConvCase{
            .name = "WhiteQueenside",
            .fen = CASTLING_WHITE,
            .uci = "e1c1",
            .move = Move(Square::E1, Square::C1, MoveFlag::CASTLE_QUEENSIDE),
        },
        ConvCase{
            .name = "BlackKingside",
            .fen = CASTLING_BLACK,
            .uci = "e8g8",
            .move = Move(Square::E8, Square::G8, MoveFlag::CASTLE_KINGSIDE),
        },
        ConvCase{
            .name = "BlackQueenside",
            .fen = CASTLING_BLACK,
            .uci = "e8c8",
            .move = Move(Square::E8, Square::C8, MoveFlag::CASTLE_QUEENSIDE),
        },
        ConvCase{
            .name = "PromoteQueen",
            .fen = PROMOTION_WHITE,
            .uci = "e7e8q",
            .move = Move(Square::E7, Square::E8, MoveFlag::PROMOTION_QUEEN),
        },
        ConvCase{
            .name = "PromoteRook",
            .fen = PROMOTION_WHITE,
            .uci = "e7e8r",
            .move = Move(Square::E7, Square::E8, MoveFlag::PROMOTION_ROOK),
        },
        ConvCase{
            .name = "PromoteBishop",
            .fen = PROMOTION_WHITE,
            .uci = "e7e8b",
            .move = Move(Square::E7, Square::E8, MoveFlag::PROMOTION_BISHOP),
        },
        ConvCase{
            .name = "PromoteKnight",
            .fen = PROMOTION_WHITE,
            .uci = "e7e8n",
            .move = Move(Square::E7, Square::E8, MoveFlag::PROMOTION_KNIGHT),
        },
        ConvCase{
            .name = "PromoteCaptureKnight",
            .fen = PROMOTION_WHITE,
            .uci = "e7d8n",
            .move = Move(Square::E7, Square::D8,
                         MoveFlag::PROMOTION_CAPTURE_KNIGHT),
        },
        ConvCase{
            .name = "BlackPromoteQueen",
            .fen = PROMOTION_BLACK,
            .uci = "e2e1q",
            .move = Move(Square::E2, Square::E1, MoveFlag::PROMOTION_QUEEN),
        }),
    [](const testing::TestParamInfo<ConvCase>& info) -> std::string {
      return info.param.name;
    });

class MoveConvRejectTest : public testing::TestWithParam<RejectCase> {};

TEST_P(MoveConvRejectTest, ReturnsNullopt) {
  const RejectCase& param = GetParam();
  const Position pos(param.fen);
  EXPECT_FALSE(uciToMove(pos, param.uci).has_value());
}

INSTANTIATE_TEST_SUITE_P(
    Rejects, MoveConvRejectTest,
    testing::Values(
        RejectCase{.name = "Unreachable", .fen = STARTPOS, .uci = "e2e5"},
        RejectCase{
            .name = "PromotionWithoutPiece",
            .fen = PROMOTION_WHITE,
            .uci = "e7e8",
        },
        RejectCase{
            .name = "PromotionOnNonPromotion",
            .fen = STARTPOS,
            .uci = "e2e4q",
        },
        RejectCase{.name = "CastleBlocked", .fen = STARTPOS, .uci = "e1g1"},
        RejectCase{.name = "Garbage", .fen = STARTPOS, .uci = "zz99"},
        RejectCase{.name = "Empty", .fen = STARTPOS, .uci = ""}),
    [](const testing::TestParamInfo<RejectCase>& info) -> std::string {
      return info.param.name;
    });

class MoveConvRoundTripTest : public testing::TestWithParam<FenCase> {};

TEST_P(MoveConvRoundTripTest, EveryGeneratedMoveRoundTrips) {
  const FenCase& param = GetParam();
  const Position pos(param.fen);
  MoveList moves;
  generatePseudoLegalMoves(pos, moves);
  ASSERT_GT(moves.size(), 0U);
  for (const Move move : moves) {
    const std::string uci = moveToUci(move);
    const std::optional<Move> parsed = uciToMove(pos, uci);
    ASSERT_TRUE(parsed.has_value()) << uci;
    if (parsed.has_value()) {
      EXPECT_EQ(*parsed, move) << uci;
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    RoundTrip, MoveConvRoundTripTest,
    testing::Values(FenCase{.name = "Startpos", .fen = STARTPOS},
                    FenCase{.name = "Kiwipete", .fen = KIWIPETE},
                    FenCase{.name = "PromotionWhite", .fen = PROMOTION_WHITE},
                    FenCase{.name = "PromotionBlack", .fen = PROMOTION_BLACK}),
    [](const testing::TestParamInfo<FenCase>& info) -> std::string {
      return info.param.name;
    });
