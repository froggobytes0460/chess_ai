#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <chess_ai/board/position.hpp>
#include <chess_ai/eval/eval.hpp>
#include <cstdint>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {
struct EvalCase {
  std::string_view name;
  std::string_view fen;
  int32_t expected;
};

struct SymmetryCase {
  std::string_view name;
  std::string_view fen;
};

// NOLINTNEXTLINE(readability-identifier-naming) GoogleTest looks up this name.
[[maybe_unused]] auto PrintTo(const EvalCase& param, std::ostream* out)
    -> void {
  *out << param.name << " [" << param.fen << ']';
}

// NOLINTNEXTLINE(readability-identifier-naming) GoogleTest looks up this name.
[[maybe_unused]] auto PrintTo(const SymmetryCase& param, std::ostream* out)
    -> void {
  *out << param.name << " [" << param.fen << ']';
}

auto swapCase(char c) -> char {
  const auto uc{static_cast<unsigned char>(c)};
  if (std::isupper(uc) != 0) {
    return static_cast<char>(std::tolower(uc));
  }
  return static_cast<char>(std::toupper(uc));
}

// Returns `fen` with colors swapped and the board flipped vertically, so the
// result is the same position seen from the other side.
auto mirrorFen(std::string_view fen) -> std::string {
  std::istringstream in{std::string(fen)};
  std::string board;
  std::string side;
  std::string castling;
  std::string en_passant;
  std::string clocks;
  in >> board >> side >> castling >> en_passant;
  std::getline(in, clocks);

  std::vector<std::string> ranks;
  std::istringstream board_in{board};
  for (std::string rank; std::getline(board_in, rank, '/');) {
    std::ranges::transform(rank, rank.begin(), swapCase);
    ranks.push_back(rank);
  }
  std::ranges::reverse(ranks);
  std::string mirrored;
  for (const std::string& rank : ranks) {
    if (!mirrored.empty()) {
      mirrored += '/';
    }
    mirrored += rank;
  }

  if (castling != "-") {
    std::ranges::transform(castling, castling.begin(), swapCase);
  }
  if (en_passant != "-") {
    en_passant[1] = en_passant[1] == '3' ? '6' : '3';
  }
  return mirrored + ' ' + (side == "w" ? "b" : "w") + ' ' + castling + ' ' +
         en_passant + clocks;
}

// Returns `fen` with the side to move swapped.
auto flipSideToMove(std::string_view fen) -> std::string {
  std::string flipped{fen};
  const size_t side{flipped.find(' ') + 1};
  flipped[side] = flipped[side] == 'w' ? 'b' : 'w';
  return flipped;
}
}  // namespace

class EvalTest : public testing::TestWithParam<EvalCase> {};

TEST_P(EvalTest, ScoresPosition) {
  const EvalCase& param = GetParam();
  const Position pos(param.fen);
  EXPECT_EQ(evaluate(pos), param.expected);
}

// Expected scores are hand-computed from the PeSTO tables. Kings on E1/E8
// cancel in all of them.
INSTANTIATE_TEST_SUITE_P(
    Positions, EvalTest,
    testing::Values(
        EvalCase{
            .name = "StartPositionIsEqual",
            .fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            .expected = 0},
        // Phase 0, pure endgame: eg pawn 94 + E2 bonus 13.
        EvalCase{.name = "PawnUpEndgameUsesEndgameScore",
                 .fen = "4k3/8/8/8/8/8/4P3/4K3 w - - 0 1",
                 .expected = 107},
        // Phase 4: mg 1025 + 10 = 1035, eg 936 - 43 = 893;
        // (1035 * 4 + 893 * 20) / 24 = 916.
        EvalCase{.name = "QueenUpTapersByPhase",
                 .fen = "4k3/8/8/8/8/8/8/3QK3 w - - 0 1",
                 .expected = 916},
        EvalCase{.name = "QueenUpSeenFromLosingSide",
                 .fen = "4k3/8/8/8/8/8/8/3QK3 b - - 0 1",
                 .expected = -916},
        // Seven queens give phase 28, clamped to 24: pure middlegame score,
        // 7 * 1025 - 124 = 7051. Unclamped blend would give 7170.
        EvalCase{.name = "PhaseClampedAfterPromotions",
                 .fen = "4k3/8/8/8/8/8/8/QQQQKQQQ w - - 0 1",
                 .expected = 7051}),
    [](const testing::TestParamInfo<EvalCase>& info) -> std::string {
      return std::string(info.param.name);
    });

class EvalSymmetryTest : public testing::TestWithParam<SymmetryCase> {};

TEST_P(EvalSymmetryTest, MirroredPositionScoresSame) {
  const SymmetryCase& param = GetParam();
  const Position pos(param.fen);
  const Position mirrored(mirrorFen(param.fen));
  EXPECT_EQ(evaluate(pos), evaluate(mirrored));
}

TEST_P(EvalSymmetryTest, FlippingSideToMoveNegatesScore) {
  const SymmetryCase& param = GetParam();
  const Position pos(param.fen);
  const Position flipped(flipSideToMove(param.fen));
  EXPECT_EQ(evaluate(pos), -evaluate(flipped));
}

INSTANTIATE_TEST_SUITE_P(
    Positions, EvalSymmetryTest,
    testing::Values(
        SymmetryCase{.name = "Kiwipete",
                     .fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/"
                            "R3K2R w KQkq - 0 1"},
        SymmetryCase{.name = "Position3",
                     .fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"},
        SymmetryCase{.name = "Position4",
                     .fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/"
                            "R2Q1RK1 w kq - 0 1"},
        SymmetryCase{.name = "Position5",
                     .fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/"
                            "RNBQK2R w KQ - 1 8"},
        SymmetryCase{.name = "EnPassantSquare",
                     .fen = "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2"}),
    [](const testing::TestParamInfo<SymmetryCase>& info) -> std::string {
      return std::string(info.param.name);
    });
