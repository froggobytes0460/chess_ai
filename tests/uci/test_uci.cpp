#include <gtest/gtest.h>

#include <chess_ai/board/position.hpp>
#include <chess_ai/uci/move_conv.hpp>
#include <chess_ai/uci/uci.hpp>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {
constexpr const char* STARTPOS{
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
};
constexpr const char* AFTER_E4{
    "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
};
constexpr const char* AFTER_E4_E5{
    "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2",
};

// Feeds `script` to the UCI loop and returns everything it printed.
auto run(std::string_view script) -> std::string {
  std::istringstream in{std::string{script}};
  std::ostringstream out;
  uciLoop(in, out);
  return out.str();
}

// Returns the move after the last "bestmove ", or "" if none was printed.
auto bestMove(const std::string& output) -> std::string {
  constexpr std::string_view KEY{"bestmove "};
  const size_t pos{output.rfind(KEY)};
  if (pos == std::string::npos) {
    return "";
  }
  const size_t begin{pos + KEY.size()};
  return output.substr(begin, output.find('\n', begin) - begin);
}

struct ResponseCase {
  const char* name{};
  const char* script{};
  const char* expected{};
  bool present{};
};

// NOLINTNEXTLINE(readability-identifier-naming) Google GTest uses this name
[[maybe_unused]] void PrintTo(const ResponseCase& param, std::ostream* out) {
  *out << param.name << " '" << param.script << '\'';
}

struct BestMoveCase {
  const char* name{};
  const char* script{};
  const char* best_move{};
};

// NOLINTNEXTLINE(readability-identifier-naming) Google GTest uses this name
[[maybe_unused]] void PrintTo(const BestMoveCase& param, std::ostream* out) {
  *out << param.name << " '" << param.script << '\'';
}

// `fen` is the position the engine should be searching after `script`.
struct LegalCase {
  const char* name{};
  const char* script{};
  const char* fen{};
};

// NOLINTNEXTLINE(readability-identifier-naming) Google GTest uses this name
[[maybe_unused]] void PrintTo(const LegalCase& param, std::ostream* out) {
  *out << param.name << " '" << param.script << '\'';
}
}  // namespace

class UciResponseTest : public testing::TestWithParam<ResponseCase> {};

TEST_P(UciResponseTest, OutputContainsExpected) {
  const ResponseCase& param = GetParam();
  const std::string output{run(param.script)};
  EXPECT_EQ(output.find(param.expected) != std::string::npos, param.present)
      << "output:\n"
      << output;
}

INSTANTIATE_TEST_SUITE_P(Responses, UciResponseTest,
                         testing::Values(
                             ResponseCase{
                                 .name = "UciOk",
                                 .script = "uci\n",
                                 .expected = "uciok",
                                 .present = true,
                             },
                             ResponseCase{
                                 .name = "IdName",
                                 .script = "uci\n",
                                 .expected = "id name ",
                                 .present = true,
                             },
                             ResponseCase{
                                 .name = "ReadyOk",
                                 .script = "isready\n",
                                 .expected = "readyok",
                                 .present = true,
                             },
                             ResponseCase{
                                 .name = "UnknownCommandIgnored",
                                 .script = "foo bar\nisready\n",
                                 .expected = "readyok",
                                 .present = true,
                             },
                             ResponseCase{
                                 .name = "QuitStopsLoop",
                                 .script = "quit\nisready\n",
                                 .expected = "readyok",
                                 .present = false,
                             },
                             ResponseCase{
                                 .name = "GoPrintsInfo",
                                 .script = "position startpos\ngo depth 1\n",
                                 .expected = "info depth 1 ",
                                 .present = true,
                             }),
                         [](const testing::TestParamInfo<ResponseCase>& info)
                             -> std::string { return info.param.name; });

TEST(UciLoopTest, EmptyInputPrintsNothing) { EXPECT_EQ(run(""), ""); }

class UciBestMoveTest : public testing::TestWithParam<BestMoveCase> {};

TEST_P(UciBestMoveTest, PrintsExpectedMove) {
  const BestMoveCase& param = GetParam();
  EXPECT_EQ(bestMove(run(param.script)), param.best_move);
}

INSTANTIATE_TEST_SUITE_P(
    BestMoves, UciBestMoveTest,
    testing::Values(
        BestMoveCase{
            .name = "BackRankMateFromFen",
            .script = "position fen 6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1\n"
                      "go depth 2\n",
            .best_move = "a1a8",
        },
        BestMoveCase{
            .name = "FenWithMoves",
            .script = "position fen 6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1 "
                      "moves a1a2 g8h8\n"
                      "go depth 2\n",
            .best_move = "a2a8",
        },
        BestMoveCase{
            .name = "StartposWithMovesFoolsMate",
            .script = "position startpos moves f2f3 e7e5 g2g4\n"
                      "go depth 2\n",
            .best_move = "d8h4",
        },
        BestMoveCase{
            .name = "CheckmatedPrintsNullMove",
            .script = "position fen rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/"
                      "RNBQKBNR w KQkq - 1 3\n"
                      "go depth 2\n",
            .best_move = "0000",
        },
        BestMoveCase{
            .name = "StalematedPrintsNullMove",
            .script = "position fen 7k/5Q2/6K1/8/8/8/8/8 b - - 0 1\n"
                      "go depth 2\n",
            .best_move = "0000",
        }),
    [](const testing::TestParamInfo<BestMoveCase>& info) -> std::string {
      return info.param.name;
    });

class UciLegalMoveTest : public testing::TestWithParam<LegalCase> {};

TEST_P(UciLegalMoveTest, BestMoveLegalInExpectedPosition) {
  const LegalCase& param = GetParam();
  const std::string output{run(param.script)};
  const std::string move{bestMove(output)};
  ASSERT_FALSE(move.empty()) << "no bestmove in output:\n" << output;
  const Position pos(param.fen);
  EXPECT_TRUE(uciToMove(pos, move).has_value())
      << "bestmove " << move << " illegal in " << param.fen;
}

INSTANTIATE_TEST_SUITE_P(
    LegalMoves, UciLegalMoveTest,
    testing::Values(
        LegalCase{
            .name = "DefaultPositionIsStartpos",
            .script = "go depth 1\n",
            .fen = STARTPOS,
        },
        LegalCase{
            .name = "StartposWithMoves",
            .script = "position startpos moves e2e4 e7e5\ngo depth 3\n",
            .fen = AFTER_E4_E5,
        },
        LegalCase{
            .name = "IllegalMoveStopsApplying",
            .script = "position startpos moves e2e4 e2e4 e7e5\ngo depth 1\n",
            .fen = AFTER_E4,
        },
        LegalCase{
            .name = "UciNewGameResets",
            .script = "position startpos moves e2e4\nucinewgame\n"
                      "go depth 1\n",
            .fen = STARTPOS,
        },
        LegalCase{
            .name = "SecondGoUsesSamePosition",
            .script = "position startpos moves e2e4\ngo depth 2\n"
                      "go depth 1\n",
            .fen = AFTER_E4,
        },
        LegalCase{
            .name = "DepthZeroClampedToOne",
            .script = "go depth 0\n",
            .fen = STARTPOS,
        },
        LegalCase{
            .name = "MoveTime",
            .script = "go movetime 200\n",
            .fen = STARTPOS,
        },
        LegalCase{
            .name = "IgnoresClockArguments",
            .script = "go wtime 1000 btime 1000 winc 0 binc 0 depth 1\n",
            .fen = STARTPOS,
        }),
    [](const testing::TestParamInfo<LegalCase>& info) -> std::string {
      return info.param.name;
    });
