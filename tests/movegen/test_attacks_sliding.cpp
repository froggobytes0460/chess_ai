#include <gtest/gtest.h>

#include <chess_ai/core/types.hpp>
#include <chess_ai/movegen/attacks_sliding.hpp>
#include <random>

namespace {

// xorshift-style random BitBoard generator for occupancy fuzzing.
auto randomBitBoard(std::mt19937_64& rng) -> BitBoard { return rng(); }

constexpr int RANDOM_OCCUPANCIES_PER_SQUARE = 1000;

}  // namespace

TEST(SlidingAttacksTest, RookMatchesBruteForceForRandomOccupancies) {
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937_64 rng(0xC0FFEEULL);
  for (int sq = 0; sq < 64; ++sq) {
    auto square = static_cast<Square>(sq);
    for (int i = 0; i < RANDOM_OCCUPANCIES_PER_SQUARE; ++i) {
      BitBoard occupied = randomBitBoard(rng);
      EXPECT_EQ(rookAttacks(square, occupied),
                detail::slidingAttacksSlow(square, occupied, detail::ROOK_DIRS))
          << "square=" << sq << " occupied=" << occupied;
    }
  }
}

TEST(SlidingAttacksTest, BishopMatchesBruteForceForRandomOccupancies) {
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937_64 rng(0xBEEFULL);
  for (int sq = 0; sq < 64; ++sq) {
    auto square = static_cast<Square>(sq);
    for (int i = 0; i < RANDOM_OCCUPANCIES_PER_SQUARE; ++i) {
      BitBoard occupied = randomBitBoard(rng);
      EXPECT_EQ(
          bishopAttacks(square, occupied),
          detail::slidingAttacksSlow(square, occupied, detail::BISHOP_DIRS))
          << "square=" << sq << " occupied=" << occupied;
    }
  }
}

TEST(SlidingAttacksTest, QueenIsUnionOfRookAndBishop) {
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937_64 rng(0xFACEULL);
  for (int sq = 0; sq < 64; ++sq) {
    auto square = static_cast<Square>(sq);
    for (int i = 0; i < RANDOM_OCCUPANCIES_PER_SQUARE; ++i) {
      BitBoard occupied = randomBitBoard(rng);
      BitBoard expected =
          detail::slidingAttacksSlow(square, occupied, detail::ROOK_DIRS) |
          detail::slidingAttacksSlow(square, occupied, detail::BISHOP_DIRS);
      EXPECT_EQ(queenAttacks(square, occupied), expected)
          << "square=" << sq << " occupied=" << occupied;
    }
  }
}
