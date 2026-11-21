#include <gtest/gtest.h>

#include <chess_ai/board/position.hpp>
#include <ostream>
#include <string>
#include <string_view>

namespace {
struct InsufficientMaterialCase {
  std::string_view name;
  std::string_view fen;
  bool not_sufficient_material;
};

// NOLINTNEXTLINE(readability-identifier-naming) GoogleTest looks up this name.
[[maybe_unused]] auto PrintTo(const InsufficientMaterialCase& param,
                              std::ostream* out) -> void {
  *out << param.name << " [" << param.fen << ']';
}
}  // namespace

class InsufficientMaterialTest
    : public ::testing::TestWithParam<InsufficientMaterialCase> {};

TEST_P(InsufficientMaterialTest, InsufficientMaterialTest) {
  const InsufficientMaterialCase& param = GetParam();
  const Position pos{param.fen};
  EXPECT_EQ(pos.isInsufficientMaterial(), param.not_sufficient_material);
}

INSTANTIATE_TEST_SUITE_P(
    InsufficientMaterialTest, InsufficientMaterialTest,
    ::testing::Values(
        InsufficientMaterialCase{
            .name = "KingVsKing",
            .fen = "8/8/2k5/8/8/4K3/8/8 w - - 0 1",
            .not_sufficient_material = true,
        },
        InsufficientMaterialCase{
            .name = "KingKnightVsKing",
            .fen = "8/8/2k5/6N1/8/4K3/8/8 w - - 0 1",
            .not_sufficient_material = true,
        },
        InsufficientMaterialCase{
            .name = "KingBishopVsKing",
            .fen = "8/8/2k5/8/3b4/4K3/8/8 w - - 0 1",
            .not_sufficient_material = true,
        },
        // Two minors total, either side: not covered by the simple rule.
        InsufficientMaterialCase{
            .name = "KingTwoKnightsVsKing",
            .fen = "8/8/2k5/6N1/8/4K3/5N2/8 w - - 0 1",
            .not_sufficient_material = false,
        },
        InsufficientMaterialCase{
            .name = "KingBishopVsKingKnight",
            .fen = "8/8/2k1n3/8/8/4K3/5B2/8 w - - 0 1",
            .not_sufficient_material = false,
        },
        InsufficientMaterialCase{
            .name = "KingQueenVsKing",
            .fen = "8/8/2k5/8/8/4K3/8/7Q w - - 0 1",
            .not_sufficient_material = false,
        },
        InsufficientMaterialCase{
            .name = "KingRookVsKing",
            .fen = "8/4K3/2k5/8/8/8/3R4/8 w - - 0 1",
            .not_sufficient_material = false,
        },
        InsufficientMaterialCase{
            .name = "KingPawnVsKing",
            .fen = "8/8/k6P/5K2/8/8/8/8 w - - 0 1",
            .not_sufficient_material = false,
        }),
    [](const ::testing::TestParamInfo<InsufficientMaterialCase>& info)
        -> std::string { return std::string(info.param.name); });
