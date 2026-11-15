#include <gtest/gtest.h>

#include <chess_ai/core/bitops.hpp>
#include <chess_ai/core/types.hpp>
#include <chess_ai/movegen/attacks.hpp>

namespace {

// Builds a bitboard from a list of squares, for expected-value readability.
auto bitboardOf(std::initializer_list<Square> squares) -> BitBoard {
  BitBoard bb{0};
  for (Square sq : squares) {
    setBit(bb, sq);
  }
  return bb;
}

using AttackFn = BitBoard (*)(Square);

struct AttackCase {
  std::string name;
  AttackFn attack_fn;
  Square square;
  BitBoard expected;
};

}  // namespace

class AttacksTest : public ::testing::TestWithParam<AttackCase> {};

TEST_P(AttacksTest, MatchesHandVerifiedSet) {
  const auto& test_case = GetParam();
  EXPECT_EQ(test_case.attack_fn(test_case.square), test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(
    Knight, AttacksTest,
    ::testing::Values(AttackCase{"CornerA1", knightAttacks, Square::A1,
                                 bitboardOf({Square::B3, Square::C2})},
                      AttackCase{"CornerH8", knightAttacks, Square::H8,
                                 bitboardOf({Square::F7, Square::G6})},
                      AttackCase{"EdgeA4", knightAttacks, Square::A4,
                                 bitboardOf({Square::B2, Square::B6, Square::C3,
                                             Square::C5})},
                      AttackCase{"CenterD4", knightAttacks, Square::D4,
                                 bitboardOf({Square::B3, Square::B5, Square::C2,
                                             Square::C6, Square::E2, Square::E6,
                                             Square::F3, Square::F5})}),
    [](const ::testing::TestParamInfo<AttackCase>& info) -> std::string {
      return info.param.name;
    });

INSTANTIATE_TEST_SUITE_P(
    King, AttacksTest,
    ::testing::Values(
        AttackCase{"CornerA1", kingAttacks, Square::A1,
                   bitboardOf({Square::A2, Square::B1, Square::B2})},
        AttackCase{"CornerH8", kingAttacks, Square::H8,
                   bitboardOf({Square::G7, Square::G8, Square::H7})},
        AttackCase{"EdgeA4", kingAttacks, Square::A4,
                   bitboardOf({Square::A3, Square::A5, Square::B3, Square::B4,
                               Square::B5})},
        AttackCase{
            "CenterD4", kingAttacks, Square::D4,
            bitboardOf({Square::C3, Square::C4, Square::C5, Square::D3,
                        Square::D5, Square::E3, Square::E4, Square::E5})}),
    [](const ::testing::TestParamInfo<AttackCase>& info) -> std::string {
      return info.param.name;
    });

TEST(AttackTablePropertiesTest, PopCountIsEightForCenterSquares) {
  EXPECT_EQ(popCount(knightAttacks(Square::D4)), 8);
  EXPECT_EQ(popCount(knightAttacks(Square::E5)), 8);
  EXPECT_EQ(popCount(kingAttacks(Square::D4)), 8);
  EXPECT_EQ(popCount(kingAttacks(Square::E5)), 8);
}

TEST(AttackTablePropertiesTest, NoSquareAttacksItself) {
  for (int sq = 0; sq < 64; ++sq) {
    auto square = static_cast<Square>(sq);
    EXPECT_FALSE(getBit(knightAttacks(square), square));
    EXPECT_FALSE(getBit(kingAttacks(square), square));
  }
}
