#include <gtest/gtest.h>

#include <algorithm>
#include <chess_ai/move.hpp>
#include <chess_ai/move_list.hpp>
#include <chess_ai/movegen.hpp>
#include <chess_ai/position.hpp>
#include <chess_ai/types.hpp>

namespace {
auto contains(const MoveList& moves, Square from, Square to, MoveFlag flag)
    -> bool {
  Move target(from, to, flag);
  return std::ranges::any_of(
      moves, [&](const Move move) -> bool { return move == target; });
}

auto countWithFrom(const MoveList& moves, Square from) -> int {
  return static_cast<int>(std::ranges::count_if(
      moves, [&](const Move move) -> bool { return move.from() == from; }));
}
}  // namespace

TEST(PawnMoveGenTest, WhiteSinglePushOnOpenBoard) {
  Position pos("8/8/8/8/8/4P3/8/8 w - - 0 1");
  MoveList moves;
  generatePawnMoves(pos, moves);
  EXPECT_TRUE(contains(moves, Square::E3, Square::E4, MoveFlag::QUIET));
  EXPECT_EQ(countWithFrom(moves, Square::E3), 1);
}

TEST(PawnMoveGenTest, WhiteDoublePushFromStartRank) {
  Position pos("8/8/8/8/8/8/4P3/8 w - - 0 1");
  MoveList moves;
  generatePawnMoves(pos, moves);
  EXPECT_TRUE(contains(moves, Square::E2, Square::E3, MoveFlag::QUIET));
  EXPECT_TRUE(contains(moves, Square::E2, Square::E4, MoveFlag::DOUBLE_PUSH));
  EXPECT_EQ(countWithFrom(moves, Square::E2), 2);
}

TEST(PawnMoveGenTest, BlackDoublePushFromStartRank) {
  Position pos("8/4p3/8/8/8/8/8/8 b - - 0 1");
  MoveList moves;
  generatePawnMoves(pos, moves);
  EXPECT_TRUE(contains(moves, Square::E7, Square::E6, MoveFlag::QUIET));
  EXPECT_TRUE(contains(moves, Square::E7, Square::E5, MoveFlag::DOUBLE_PUSH));
  EXPECT_EQ(countWithFrom(moves, Square::E7), 2);
}

TEST(PawnMoveGenTest, PushBlockedByOccupiedSquareGeneratesNothing) {
  Position pos("8/8/8/8/4p3/4P3/8/8 w - - 0 1");
  MoveList moves;
  generatePawnMoves(pos, moves);
  EXPECT_EQ(countWithFrom(moves, Square::E3), 0);
}

TEST(PawnMoveGenTest, DoublePushBlockedTwoAheadStillAllowsSinglePush) {
  Position pos("8/8/8/8/4p3/8/4P3/8 w - - 0 1");
  MoveList moves;
  generatePawnMoves(pos, moves);
  EXPECT_TRUE(contains(moves, Square::E2, Square::E3, MoveFlag::QUIET));
  EXPECT_FALSE(contains(moves, Square::E2, Square::E4, MoveFlag::DOUBLE_PUSH));
  EXPECT_EQ(countWithFrom(moves, Square::E2), 1);
}

TEST(PawnMoveGenTest, WhitePushPromotesOnRank8) {
  Position pos("8/4P3/8/8/8/8/8/8 w - - 0 1");
  MoveList moves;
  generatePawnMoves(pos, moves);
  EXPECT_TRUE(
      contains(moves, Square::E7, Square::E8, MoveFlag::PROMOTION_KNIGHT));
  EXPECT_TRUE(
      contains(moves, Square::E7, Square::E8, MoveFlag::PROMOTION_BISHOP));
  EXPECT_TRUE(
      contains(moves, Square::E7, Square::E8, MoveFlag::PROMOTION_ROOK));
  EXPECT_TRUE(
      contains(moves, Square::E7, Square::E8, MoveFlag::PROMOTION_QUEEN));
  EXPECT_EQ(countWithFrom(moves, Square::E7), 4);
}

TEST(PawnMoveGenTest, BlackPushPromotesOnRank1) {
  Position pos("8/8/8/8/8/8/4p3/8 b - - 0 1");
  MoveList moves;
  generatePawnMoves(pos, moves);
  EXPECT_TRUE(
      contains(moves, Square::E2, Square::E1, MoveFlag::PROMOTION_QUEEN));
  EXPECT_EQ(countWithFrom(moves, Square::E2), 4);
}

TEST(PawnMoveGenTest, CapturesBothDiagonalsAvailable) {
  Position pos("8/8/8/3pppp1/4P3/8/8/8 w - - 0 1");
  MoveList moves;
  generatePawnMoves(pos, moves);
  EXPECT_TRUE(contains(moves, Square::E4, Square::D5, MoveFlag::CAPTURE));
  EXPECT_TRUE(contains(moves, Square::E4, Square::F5, MoveFlag::CAPTURE));
  EXPECT_EQ(countWithFrom(moves, Square::E4), 2);
}

TEST(PawnMoveGenTest, AFilePawnHasNoLeftCaptureNoWraparound) {
  // Enemy pawn placed on H3: if file-wraparound existed in the attack table,
  // A2's "left" capture target would incorrectly land on H3.
  Position pos("8/8/8/8/8/7p/P7/8 w - - 0 1");
  MoveList moves;
  generatePawnMoves(pos, moves);
  for (Move move : moves) {
    if (move.from() == Square::A2) {
      EXPECT_FALSE(move.isCapture());
    }
  }
}

TEST(PawnMoveGenTest, HFilePawnCapturesOnlyLeftDiagonal) {
  Position pos("8/8/8/7p/6PP/8/8/8 b - - 0 1");
  MoveList moves;
  generatePawnMoves(pos, moves);
  EXPECT_TRUE(contains(moves, Square::H5, Square::G4, MoveFlag::CAPTURE));
  EXPECT_EQ(countWithFrom(moves, Square::H5), 1);
}

TEST(PawnMoveGenTest, CapturePromotesOnBackRank) {
  Position pos("3n4/4P3/8/8/8/8/8/8 w - - 0 1");
  MoveList moves;
  generatePawnMoves(pos, moves);
  EXPECT_TRUE(contains(moves, Square::E7, Square::D8,
                       MoveFlag::PROMOTION_CAPTURE_QUEEN));
  EXPECT_TRUE(
      contains(moves, Square::E7, Square::E8, MoveFlag::PROMOTION_QUEEN));
  EXPECT_EQ(countWithFrom(moves, Square::E7), 8);
}

TEST(PawnMoveGenTest, EnPassantCaptureGeneratedWhenEpSquareSet) {
  Position pos("8/8/8/3pP3/8/8/8/8 w - d6 0 1");
  MoveList moves;
  generatePawnMoves(pos, moves);
  EXPECT_TRUE(contains(moves, Square::E5, Square::D6, MoveFlag::EN_PASSANT));
}

TEST(PawnMoveGenTest, NoEnPassantWhenEpSquareNotSet) {
  Position pos("8/8/8/3pP3/8/8/8/8 w - - 0 1");
  MoveList moves;
  generatePawnMoves(pos, moves);
  EXPECT_FALSE(contains(moves, Square::E5, Square::D6, MoveFlag::EN_PASSANT));
}
