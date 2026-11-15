#include <gtest/gtest.h>

#include <chess_ai/core/move.hpp>
#include <chess_ai/core/types.hpp>

TEST(MoveTest, DefaultConstructedIsZero) {
  Move move;
  EXPECT_EQ(move.raw(), 0);
}

TEST(MoveTest, EncodesFromToFlag) {
  Move move(Square::E2, Square::E4, MoveFlag::DOUBLE_PUSH);
  EXPECT_EQ(move.from(), Square::E2);
  EXPECT_EQ(move.to(), Square::E4);
  EXPECT_EQ(move.flag(), MoveFlag::DOUBLE_PUSH);
}

TEST(MoveTest, CastleFlagsRoundTrip) {
  Move kingside(Square::E1, Square::G1, MoveFlag::CASTLE_KINGSIDE);
  Move queenside(Square::E1, Square::C1, MoveFlag::CASTLE_QUEENSIDE);
  EXPECT_EQ(kingside.flag(), MoveFlag::CASTLE_KINGSIDE);
  EXPECT_EQ(queenside.flag(), MoveFlag::CASTLE_QUEENSIDE);
}

TEST(MoveTest, EqualityOperators) {
  Move first(Square::B1, Square::C3, MoveFlag::QUIET);
  Move second(Square::B1, Square::C3, MoveFlag::QUIET);
  Move different(Square::B1, Square::D2, MoveFlag::QUIET);
  EXPECT_EQ(first, second);
  EXPECT_NE(first, different);
}

struct MoveFlagCase {
  MoveFlag flag;
  bool is_capture;
  bool is_promotion;
};

class MoveFlagTest : public ::testing::TestWithParam<MoveFlagCase> {};

TEST_P(MoveFlagTest, CaptureAndPromotionBits) {
  const auto& test_case = GetParam();
  Move move(Square::E7, Square::D8, test_case.flag);
  EXPECT_EQ(move.isCapture(), test_case.is_capture);
  EXPECT_EQ(move.isPromotion(), test_case.is_promotion);
}

INSTANTIATE_TEST_SUITE_P(
    MoveFlags, MoveFlagTest,
    ::testing::Values(MoveFlagCase{MoveFlag::QUIET, false, false},
                      MoveFlagCase{MoveFlag::CAPTURE, true, false},
                      MoveFlagCase{MoveFlag::EN_PASSANT, true, false},
                      MoveFlagCase{MoveFlag::PROMOTION_QUEEN, false, true},
                      MoveFlagCase{MoveFlag::PROMOTION_CAPTURE_QUEEN, true,
                                   true}),
    [](const ::testing::TestParamInfo<MoveFlagCase>& info) -> std::string {
      switch (info.param.flag) {
        case MoveFlag::QUIET:
          return "Quiet";
        case MoveFlag::CAPTURE:
          return "Capture";
        case MoveFlag::EN_PASSANT:
          return "EnPassant";
        case MoveFlag::PROMOTION_QUEEN:
          return "PromotionQueen";
        case MoveFlag::PROMOTION_CAPTURE_QUEEN:
          return "PromotionCaptureQueen";
        default:
          return "Unknown";
      }
    });
