#include <gtest/gtest.h>

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

// Same low bits (same slot), different top 16 bits (different position).
constexpr uint64_t KEY{0x1234000000000042};
constexpr uint64_t SAME_SLOT_KEY{0x5678000000000042};
// Same top 16 bits as KEY, other low bits: aliases KEY only in a one-slot
// table.
constexpr uint64_t SAME_VERIFY_KEY{0x1234000000000043};

constexpr Move MOVE_A{Square::E2, Square::E4, MoveFlag::DOUBLE_PUSH};
constexpr Move MOVE_B{Square::G1, Square::F3, MoveFlag::QUIET};

struct RoundTripCase {
  const char* name{};
  uint64_t key{};
  Move move;
  int32_t score{};
  int depth{};
  Bound bound{Bound::NONE};
};

// NOLINTNEXTLINE(readability-identifier-naming) Google GTest uses this name
[[maybe_unused]] void PrintTo(const RoundTripCase& param, std::ostream* out) {
  *out << param.name;
}
}  // namespace

class TTRoundTripTest : public testing::TestWithParam<RoundTripCase> {};

TEST_P(TTRoundTripTest, StoreThenProbeReturnsSameEntry) {
  const RoundTripCase& param = GetParam();
  TranspositionTable tt{TABLE_MB};
  tt.store(param.key, param.move, param.score, param.depth, param.bound);

  const std::optional<TTEntry> entry = tt.probe(param.key);
  ASSERT_TRUE(entry.has_value());
  if (!entry.has_value()) {
    return;
  }
  EXPECT_EQ(entry->move, param.move);
  EXPECT_EQ(entry->score, param.score);
  EXPECT_EQ(entry->depth, param.depth);
  EXPECT_EQ(entry->bound, param.bound);
}

INSTANTIATE_TEST_SUITE_P(TT, TTRoundTripTest,
                         testing::Values(
                             RoundTripCase{
                                 .name = "Exact",
                                 .key = KEY,
                                 .move = MOVE_A,
                                 .score = 35,
                                 .depth = 5,
                                 .bound = Bound::EXACT,
                             },
                             RoundTripCase{
                                 .name = "Lower",
                                 .key = KEY,
                                 .move = MOVE_B,
                                 .score = -120,
                                 .depth = 1,
                                 .bound = Bound::LOWER,
                             },
                             RoundTripCase{
                                 .name = "UpperNoMove",
                                 .key = KEY,
                                 .move = Move{},
                                 .score = 0,
                                 .depth = 3,
                                 .bound = Bound::UPPER,
                             },
                             RoundTripCase{
                                 .name = "NegativeMateScore",
                                 .key = KEY,
                                 .move = MOVE_A,
                                 .score = -MATE + 5,
                                 .depth = 7,
                                 .bound = Bound::EXACT,
                             },
                             RoundTripCase{
                                 .name = "PositiveMateScore",
                                 .key = SAME_SLOT_KEY,
                                 .move = MOVE_B,
                                 .score = MATE - 3,
                                 .depth = 12,
                                 .bound = Bound::LOWER,
                             }),
                         [](const testing::TestParamInfo<RoundTripCase>& info)
                             -> std::string { return info.param.name; });

TEST(TTTest, FreshTableMisses) {
  const TranspositionTable tt{TABLE_MB};
  EXPECT_FALSE(tt.probe(KEY).has_value());
  EXPECT_FALSE(tt.probe(0).has_value());
}

TEST(TTTest, SameSlotDifferentKeyMisses) {
  TranspositionTable tt{TABLE_MB};
  tt.store(KEY, MOVE_A, 10, 4, Bound::EXACT);
  EXPECT_FALSE(tt.probe(SAME_SLOT_KEY).has_value());
}

TEST(TTTest, SameSlotDifferentKeyReplaces) {
  TranspositionTable tt{TABLE_MB};
  tt.store(KEY, MOVE_A, 10, 4, Bound::EXACT);
  tt.store(SAME_SLOT_KEY, MOVE_B, 20, 2, Bound::LOWER);
  EXPECT_FALSE(tt.probe(KEY).has_value());
  ASSERT_TRUE(tt.probe(SAME_SLOT_KEY).has_value());
}

TEST(TTTest, ClearEmptiesTable) {
  TranspositionTable tt{TABLE_MB};
  tt.store(KEY, MOVE_A, 10, 4, Bound::EXACT);
  tt.clear();
  EXPECT_FALSE(tt.probe(KEY).has_value());
}

TEST(TTTest, NullMoveKeepsOldMoveForSameKey) {
  TranspositionTable tt{TABLE_MB};
  tt.store(KEY, MOVE_A, 10, 4, Bound::EXACT);
  tt.store(KEY, Move{}, -5, 6, Bound::UPPER);

  const std::optional<TTEntry> entry = tt.probe(KEY);
  ASSERT_TRUE(entry.has_value());
  if (!entry.has_value()) {
    return;
  }
  EXPECT_EQ(entry->move, MOVE_A);
  EXPECT_EQ(entry->score, -5);
  EXPECT_EQ(entry->depth, 6);
  EXPECT_EQ(entry->bound, Bound::UPPER);
}

TEST(TTTest, NullMoveClearsMoveForOtherKey) {
  TranspositionTable tt{TABLE_MB};
  tt.store(KEY, MOVE_A, 10, 4, Bound::EXACT);
  tt.store(SAME_SLOT_KEY, Move{}, -5, 6, Bound::UPPER);

  const std::optional<TTEntry> entry = tt.probe(SAME_SLOT_KEY);
  ASSERT_TRUE(entry.has_value());
  if (entry.has_value()) {
    EXPECT_EQ(entry->move, Move{});
  }
}

TEST(TTTest, ZeroSizeTableStillWorks) {
  TranspositionTable tt{0};
  tt.store(KEY, MOVE_A, 10, 4, Bound::EXACT);
  ASSERT_TRUE(tt.probe(KEY).has_value());
  EXPECT_FALSE(tt.probe(SAME_SLOT_KEY).has_value());
}

TEST(TTTest, ResizeEmptiesTable) {
  TranspositionTable tt{TABLE_MB};
  tt.store(KEY, MOVE_A, 10, 4, Bound::EXACT);
  tt.resize(TABLE_MB);
  EXPECT_FALSE(tt.probe(KEY).has_value());
}

TEST(TTTest, ResizeChangesSlotCount) {
  TranspositionTable tt{0};
  tt.store(KEY, MOVE_A, 10, 4, Bound::EXACT);
  ASSERT_TRUE(tt.probe(SAME_VERIFY_KEY).has_value());

  tt.resize(TABLE_MB);
  tt.store(KEY, MOVE_A, 10, 4, Bound::EXACT);
  EXPECT_TRUE(tt.probe(KEY).has_value());
  EXPECT_FALSE(tt.probe(SAME_VERIFY_KEY).has_value());
}
