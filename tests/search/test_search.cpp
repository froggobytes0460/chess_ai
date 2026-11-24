#include <gtest/gtest.h>

#include <atomic>
#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move.hpp>
#include <chess_ai/core/types.hpp>
#include <chess_ai/search/search.hpp>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

namespace {
constexpr int32_t MATE{31000};

struct SearchCase {
  const char* name{};
  const char* fen{};
  std::optional<int32_t>
      expected_score{};  // NOLINT(readability-redundant-member-init)
                         // GGC compliance
  int16_t depth{};
  std::optional<Move>
      best_move{};  // NOLINT(readability-redundant-member-init) GGC compliance
};

// NOLINTNEXTLINE(readability-identifier-naming) Google GTest uses this name
[[maybe_unused]] void PrintTo(const SearchCase& param, std::ostream* out) {
  *out << param.name << '[' << param.fen << "]_D" << param.depth;
}

// Long algebraic (UCI) notation, e.g. "e2e4", "e7e8q".
auto toUci(Move move) -> std::string {
  const auto square = [](Square sq) -> std::string {
    const int index{static_cast<int>(sq)};
    return std::string{
        static_cast<char>('a' + (index % 8)),
        static_cast<char>('1' + (index / 8)),
    };
  };
  std::string uci{square(move.from()) + square(move.to())};
  if (move.isPromotion()) {
    // Indexed by `PieceType` (KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN).
    constexpr std::string_view PIECE_CHARS{"kqrbnp"};
    uci += PIECE_CHARS[static_cast<size_t>(move.promotionType())];
  }
  return uci;
}
}  // namespace

// Global, not in the anonymous namespace: GTest finds printers for `Move` via
// ADL, which only searches `Move`'s namespace (global). Not `static`: clang
// flags it unused, since only GTest's printer templates call it.
auto operator<<(std::ostream& out, Move move) -> std::ostream& {
  return out << toUci(move);
}

class SearchTest : public testing::TestWithParam<SearchCase> {};

TEST_P(SearchTest, SearchTest) {
  const SearchCase& param = GetParam();
  Position pos(param.fen);
  std::atomic<bool> stop{false};
  SearchResult res =
      search(pos, {.depth = static_cast<int>(param.depth)}, stop);

  if (param.best_move.has_value()) {
    EXPECT_EQ(res.best_move, param.best_move);
  }
  if (param.expected_score.has_value()) {
    EXPECT_EQ(res.score, param.expected_score);
  }
}

INSTANTIATE_TEST_SUITE_P(
    Search, SearchTest,
    testing::Values(
        SearchCase{
            .name = "MateInOne",
            .fen = "6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1",
            .expected_score = MATE - 1,
            .depth = 2,
            .best_move = Move(Square::A1, Square::A8, MoveFlag::QUIET),
        },
        SearchCase{
            .name = "HangingQueen",
            .fen = "4k3/8/8/3q4/8/8/8/3QK3 w - - 0 1",
            .depth = 2,
            .best_move = Move(Square::D1, Square::D5, MoveFlag::CAPTURE),
        },
        SearchCase{
            .name = "CheckmatedPosition",
            .fen = "R5k1/5ppp/8/8/8/8/8/6K1 b - - 0 1",
            .expected_score = -MATE,
            .depth = 2,
        },
        SearchCase{
            .name = "StalematedPosition",
            .fen = "8/8/8/8/8/8/p1K5/k1N5 b - - 0 1",
            .expected_score = 0,
            .depth = 2,
        },
        // Queen up, but no capture or pawn move resets the clock: every
        // child is a 50-move draw.
        SearchCase{
            .name = "FiftyMoveRuleDraw",
            .fen = "4k3/8/8/8/8/8/8/3QK3 w - - 100 80",
            .expected_score = 0,
            .depth = 2,
        },
        // Knight up, but every child is KNvK: insufficient material.
        SearchCase{
            .name = "InsufficientMaterialDraw",
            .fen = "8/8/2k5/6N1/8/4K3/8/8 w - - 0 1",
            .expected_score = 0,
            .depth = 2,
        },
        // Quiescence: Qxd5 wins a knight but exd5 loses the queen; only
        // qsearch sees the recapture at depth 1. Rxa7 wins a pawn safely.
        SearchCase{
            .name = "QuiescenceAvoidsDefendedCapture",
            .fen = "7k/p7/4p3/3n4/8/8/8/R2Q2K1 w - - 0 1",
            .depth = 1,
            .best_move = Move(Square::A1, Square::A7, MoveFlag::CAPTURE),
        },
        // Quiescence: Rxa6 wins a knight but leaves the queen to exd4; only
        // qsearch sees the reply at depth 1. Qxe5 removes the attacker.
        SearchCase{
            .name = "QuiescenceSavesAttackedQueen",
            .fen = "8/7k/n7/4p3/3Q4/8/8/R5K1 w - - 0 1",
            .depth = 1,
            .best_move = Move(Square::D4, Square::E5, MoveFlag::CAPTURE),
        }),
    [](const testing::TestParamInfo<SearchCase>& info) -> std::string {
      return info.param.name;
    });
