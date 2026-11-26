#include <gtest/gtest.h>

#include <atomic>
#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move.hpp>
#include <chess_ai/core/types.hpp>
#include <chess_ai/search/search.hpp>
#include <chess_ai/search/tt.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>

namespace {
constexpr size_t TABLE_MB{1};

struct TTSearchCase {
  const char* name{};
  const char* fen{};
  int depth{};
  std::optional<int32_t>
      expected_score{};  // NOLINT(readability-redundant-member-init)
                         // GGC compliance
  std::optional<Move>
      best_move{};  // NOLINT(readability-redundant-member-init) GGC compliance
};

// NOLINTNEXTLINE(readability-identifier-naming) Google GTest uses this name
[[maybe_unused]] void PrintTo(const TTSearchCase& param, std::ostream* out) {
  *out << param.name << '[' << param.fen << "]_D" << param.depth;
}

auto runSearch(const TTSearchCase& param, TranspositionTable& tt)
    -> SearchResult {
  Position pos(param.fen);
  std::atomic<bool> stop{false};
  return search(pos, {.depth = param.depth}, tt, stop);
}
}  // namespace

class TTSearchTest : public testing::TestWithParam<TTSearchCase> {};

// Second search reuses the first one's entries: results must not change.
// Mate cases check ply adjustment of mate scores stored in the TT.
TEST_P(TTSearchTest, WarmSearchKeepsResult) {
  const TTSearchCase& param = GetParam();
  TranspositionTable tt{TABLE_MB};
  const SearchResult cold{runSearch(param, tt)};
  const SearchResult warm{runSearch(param, tt)};

  for (const SearchResult& res : {cold, warm}) {
    if (param.best_move.has_value()) {
      EXPECT_EQ(res.best_move, param.best_move);
    }
    if (param.expected_score.has_value()) {
      EXPECT_EQ(res.score, param.expected_score);
    }
  }
}

TEST_P(TTSearchTest, WarmSearchUsesFewerNodes) {
  const TTSearchCase& param = GetParam();
  TranspositionTable tt{TABLE_MB};
  const SearchResult cold{runSearch(param, tt)};
  const SearchResult warm{runSearch(param, tt)};

  EXPECT_LT(warm.nodes, cold.nodes);
}

// Cleared table must behave like a fresh one.
TEST_P(TTSearchTest, ClearMatchesFreshTable) {
  const TTSearchCase& param = GetParam();
  TranspositionTable fresh{TABLE_MB};
  const SearchResult expected{runSearch(param, fresh)};

  TranspositionTable used{TABLE_MB};
  [[maybe_unused]] const SearchResult warmup{runSearch(param, used)};
  used.clear();
  const SearchResult res{runSearch(param, used)};

  EXPECT_EQ(res.nodes, expected.nodes);
  EXPECT_EQ(res.score, expected.score);
  EXPECT_EQ(res.best_move, expected.best_move);
}

INSTANTIATE_TEST_SUITE_P(
    TTSearch, TTSearchTest,
    testing::Values(
        TTSearchCase{
            .name = "MateInOne",
            .fen = "6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1",
            .depth = 4,
            .expected_score = MATE - 1,
            .best_move = Move(Square::A1, Square::A8, MoveFlag::QUIET),
        },
        // Nf6+ gxf6 Bxf7#.
        TTSearchCase{
            .name = "MateInTwo",
            .fen = "r2qkb1r/pp2nppp/3p4/2pNN1B1/2BnP3/3P4/PPP2PPP/"
                   "R2bK2R w KQkq - 1 1",
            .depth = 4,
            .expected_score = MATE - 3,
            .best_move = Move(Square::D5, Square::F6, MoveFlag::QUIET),
        },
        TTSearchCase{
            .name = "HangingQueen",
            .fen = "4k3/8/8/3q4/8/8/8/3QK3 w - - 0 1",
            .depth = 4,
            .best_move = Move(Square::D1, Square::D5, MoveFlag::CAPTURE),
        },
        TTSearchCase{
            .name = "StartPos",
            .fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            .depth = 4,
        }),
    [](const testing::TestParamInfo<TTSearchCase>& info) -> std::string {
      return info.param.name;
    });
