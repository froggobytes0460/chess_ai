#include <gtest/gtest.h>

#include <chess_ai/board/position.hpp>
#include <chess_ai/perft/perft.hpp>
#include <cstdint>
#include <string>
#include <string_view>

struct PerftCase {
  const char* name;
  const char* fen;
  uint64_t expected;
  int depth;
};

class PerftTest : public ::testing::TestWithParam<PerftCase> {};

TEST_P(PerftTest, PerftTest) {
  Position pos{GetParam().fen};
  EXPECT_EQ(perft(pos, GetParam().depth), GetParam().expected);
}

auto namePerftCase(const ::testing::TestParamInfo<PerftCase>& info)
    -> std::string {
  return std::string{info.param.name} + "_D" + std::to_string(info.param.depth);
}

// Reference counts: https://www.chessprogramming.org/Perft_Results
// NOLINTBEGIN(readability-trailing-comma)
constexpr const char* STARTPOS{
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"};
constexpr const char* KIWIPETE{
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"};
constexpr const char* POS3{"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"};
constexpr const char* POS4{
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"};
constexpr const char* POS5{
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"};
constexpr const char* POS6{
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"};
// NOLINTEND(readability-trailing-comma)

INSTANTIATE_TEST_SUITE_P(
    Performance, PerftTest,
    ::testing::Values(
        PerftCase{"Startpos", STARTPOS, 20, 1},
        PerftCase{"Startpos", STARTPOS, 400, 2},
        PerftCase{"Startpos", STARTPOS, 8902, 3},
        PerftCase{"Startpos", STARTPOS, 197281, 4},
        PerftCase{"Startpos", STARTPOS, 4865609, 5},
        PerftCase{"Kiwipete", KIWIPETE, 48, 1},
        PerftCase{"Kiwipete", KIWIPETE, 2039, 2},
        PerftCase{"Kiwipete", KIWIPETE, 97862, 3},
        PerftCase{"Kiwipete", KIWIPETE, 4085603, 4},
        PerftCase{"Pos3", POS3, 14, 1}, PerftCase{"Pos3", POS3, 191, 2},
        PerftCase{"Pos3", POS3, 2812, 3}, PerftCase{"Pos3", POS3, 43238, 4},
        PerftCase{"Pos3", POS3, 674624, 5}, PerftCase{"Pos4", POS4, 6, 1},
        PerftCase{"Pos4", POS4, 264, 2}, PerftCase{"Pos4", POS4, 9467, 3},
        PerftCase{"Pos4", POS4, 422333, 4}, PerftCase{"Pos5", POS5, 44, 1},
        PerftCase{"Pos5", POS5, 1486, 2}, PerftCase{"Pos5", POS5, 62379, 3},
        PerftCase{"Pos5", POS5, 2103487, 4}, PerftCase{"Pos6", POS6, 46, 1},
        PerftCase{"Pos6", POS6, 2079, 2}, PerftCase{"Pos6", POS6, 89890, 3},
        PerftCase{"Pos6", POS6, 3894594, 4}),
    namePerftCase);
