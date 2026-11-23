#include <gtest/gtest.h>

#include <algorithm>
#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move_list.hpp>
#include <chess_ai/movegen/movegen.hpp>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace {
struct CaptureGenCase {
  std::string_view name;
  std::string_view fen;
};

// NOLINTNEXTLINE(readability-identifier-naming) Google GTest uses this name
[[maybe_unused]] void PrintTo(const CaptureGenCase& param, std::ostream* out) {
  *out << param.name << " [" << param.fen << ']';
}

auto sortedMovesRaw(const MoveList& list) -> std::vector<uint16_t> {
  std::vector<uint16_t> out{};
  out.reserve(list.size());
  for (const Move move : list) {
    out.push_back(move.raw());
  }

  std::ranges::sort(out);
  return out;
}
class CaptureGenTest : public ::testing::TestWithParam<CaptureGenCase> {};
}  // namespace

TEST_P(CaptureGenTest, MatchesFilteredFullGen) {
  Position pos{GetParam().fen};

  MoveList all;
  generatePseudoLegalMoves(pos, all);

  MoveList expected;
  for (const Move move : all) {
    if (move.isCapture() || move.isPromotion()) {
      expected.push(move);
    }
  }

  MoveList actual;
  generateCaptureMoves(pos, actual);

  EXPECT_EQ(sortedMovesRaw(actual), sortedMovesRaw(expected));
}

INSTANTIATE_TEST_SUITE_P(
    PseudoLegal, CaptureGenTest,
    ::testing::Values(
        CaptureGenCase{
            .name = "NoCapturesAndPromotions",
            .fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        },
        CaptureGenCase{
            .name = "PawnFirstRankNonPromotionMoves",
            .fen = "4k3/1p6/8/8/8/8/1P6/4K3 w - - 0 1",
        },
        CaptureGenCase{
            .name = "EnPassantSquareSetButNoCaptureMoves",
            .fen =
                "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
        },
        CaptureGenCase{
            .name = "PawnFirstRankNonPromotionMovesBlack",
            .fen = "4k3/1p6/8/8/8/8/1P6/4K3 b - - 0 1",
        },
        CaptureGenCase{
            .name = "QuietPromotions",
            .fen = "4k3/P7/8/8/8/8/8/4K3 w - - 0 1",
        },
        CaptureGenCase{
            .name = "BlockedPushCapturePromotionEdgeFile",
            .fen = "nr2k3/P6p/8/8/8/8/8/4K3 w - - 0 1",
        },
        CaptureGenCase{
            .name = "PushAndBothSideCapturePromotions",
            .fen = "1n1r2k1/2P5/8/8/8/8/8/4K3 w - - 0 1",
        },
        CaptureGenCase{
            .name = "BlackPromotions",
            .fen = "4k3/8/8/8/8/8/p7/1N2K3 b - - 0 1",
        },
        CaptureGenCase{
            .name = "EnPassantWhite",
            .fen =
                "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3",
        },
        CaptureGenCase{
            .name = "EnPassantBlack",
            .fen = "4k3/8/8/8/3Pp3/8/8/4K3 b - d3 0 1",
        },
        CaptureGenCase{
            .name = "EnPassantFromAFile",
            .fen = "4k3/8/8/Pp6/8/8/8/4K3 w - b6 0 1",
        },
        CaptureGenCase{
            .name = "EnPassantFromHFile",
            .fen = "4k3/8/8/6pP/8/8/8/4K3 w - g6 0 1",
        },
        CaptureGenCase{
            .name = "CastlingExcluded",
            .fen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
        },
        CaptureGenCase{
            .name = "Kiwipete",
            .fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w "
                   "KQkq - 0 1",
        },
        CaptureGenCase{
            .name = "PerftPosition3",
            .fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        },
        CaptureGenCase{
            .name = "PerftPosition4",
            .fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq "
                   "- 0 1",
        },
        CaptureGenCase{
            .name = "PerftPosition5",
            .fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        }),
    [](const ::testing::TestParamInfo<CaptureGenCase>& info) -> std::string {
      return std::string(info.param.name);
    });
