#include <gtest/gtest.h>

#include <atomic>
#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move.hpp>
#include <chess_ai/core/move_list.hpp>
#include <chess_ai/core/types.hpp>
#include <chess_ai/movegen/movegen.hpp>
#include <chess_ai/search/search.hpp>
#include <chess_ai/search/tt.hpp>
#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <ostream>
#include <string>

namespace {
// Search checks node and time limits only every 2048 nodes.
constexpr uint64_t NODE_CHECK_INTERVAL{2048};
constexpr uint64_t NO_NODE_LIMIT{std::numeric_limits<uint64_t>::max()};

constexpr const char* STARTPOS{
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
};
constexpr const char* KIWIPETE{
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
};

struct LimitCase {
  const char* name{};
  const char* fen{};
  uint64_t nodes{NO_NODE_LIMIT};
  bool deadline_passed{false};
  bool stop_set{false};
  std::optional<int16_t>
      max_depth{};  // NOLINT(readability-redundant-member-init) GCC compliance
};

// NOLINTNEXTLINE(readability-identifier-naming) Google GTest uses this name
[[maybe_unused]] void PrintTo(const LimitCase& param, std::ostream* out) {
  *out << param.name << '[' << param.fen << ']';
}

auto isLegal(Position& pos, Move move) -> bool {
  MoveList moves;
  generatePseudoLegalMoves(pos, moves);
  for (const Move candidate : moves) {
    if (candidate != move) {
      continue;
    }
    const MoveState state = pos.makeMove(move);
    const bool legal = !inCheck(pos, opposite(pos.sideToMove()));
    pos.undoMove(move, state);
    return legal;
  }
  return false;
}
}  // namespace

class SearchLimitsTest : public testing::TestWithParam<LimitCase> {};

TEST_P(SearchLimitsTest, StopsWithLegalMoveAndRestoredPosition) {
  const LimitCase& param = GetParam();
  Position pos(param.fen);
  const std::string fen_before{pos.toFEN()};
  const uint64_t key_before{pos.zobristKey()};

  SearchLimits limits{.nodes = param.nodes};
  TranspositionTable tt{1};
  if (param.deadline_passed) {
    limits.deadline = std::chrono::steady_clock::now();
  }
  std::atomic<bool> stop{param.stop_set};
  const SearchResult res = search(pos, limits, tt, stop);

  EXPECT_EQ(pos.toFEN(), fen_before);
  EXPECT_EQ(pos.zobristKey(), key_before);
  EXPECT_GE(res.depth, 1);
  EXPECT_TRUE(isLegal(pos, res.best_move));
  if (param.max_depth.has_value()) {
    EXPECT_LE(res.depth, *param.max_depth);
  }
  if (param.nodes != NO_NODE_LIMIT) {
    EXPECT_GE(res.nodes, param.nodes);
    EXPECT_LE(res.nodes, param.nodes + NODE_CHECK_INTERVAL);
  }
}

INSTANTIATE_TEST_SUITE_P(
    Limits, SearchLimitsTest,
    testing::Values(
        // Depth 1 always completes, depth 2 aborts on its first node.
        LimitCase{
            .name = "StopSetBeforeSearchStartpos",
            .fen = STARTPOS,
            .stop_set = true,
            .max_depth = 1,
        },
        LimitCase{
            .name = "StopSetBeforeSearchKiwipete",
            .fen = KIWIPETE,
            .stop_set = true,
            .max_depth = 1,
        },
        // Deadline is only checked every 2048 nodes; startpos reaches that
        // count before depth 3 completes.
        LimitCase{
            .name = "DeadlinePassedStartpos",
            .fen = STARTPOS,
            .deadline_passed = true,
            .max_depth = 2,
        },
        LimitCase{
            .name = "NodeLimitStartpos",
            .fen = STARTPOS,
            .nodes = 10'000,
        },
        LimitCase{
            .name = "NodeLimitKiwipete",
            .fen = KIWIPETE,
            .nodes = 10'000,
        }),
    [](const testing::TestParamInfo<LimitCase>& info) -> std::string {
      return info.param.name;
    });
