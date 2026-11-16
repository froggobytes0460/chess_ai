#include <array>
#include <chess_ai/board/position.hpp>
#include <chess_ai/perft/perft.hpp>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <iostream>

namespace {

struct BenchCase {
  const char* name;
  const char* fen;
  int depth;
  uint64_t expected;
};

struct BenchResult {
  uint64_t nodes;
  double seconds;
};

// Reference counts: https://www.chessprogramming.org/Perft_Results
constexpr std::array<BenchCase, 3> CASES{
    {
        {
            .name = "Startpos",
            .fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            .depth = 6,
            .expected = 119060324,
        },
        {
            .name = "Kiwipete",
            .fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w "
                   "KQkq - 0 1",
            .depth = 5,
            .expected = 193690690,
        },
        {
            .name = "Pos3",
            .fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
            .depth = 6,
            .expected = 11030083,
        },
    },
};

constexpr int REPEATS{3};

// Runs perft once on a fresh position and measures wall-clock time.
auto timePerft(const BenchCase& bench) -> BenchResult {
  Position pos{bench.fen};

  const auto start{std::chrono::steady_clock::now()};
  const uint64_t nodes{perft(pos, bench.depth)};
  const auto end{std::chrono::steady_clock::now()};

  return {
      .nodes = nodes,
      .seconds = std::chrono::duration<double>(end - start).count(),
  };
}

}  // namespace

auto main() -> int {
  uint64_t total_nodes{0};
  double total_seconds{0.0};

  for (const BenchCase& bench : CASES) {
    BenchResult best{.nodes = 0, .seconds = 0.0};
    for (int i = 0; i < REPEATS; ++i) {
      const BenchResult run{timePerft(bench)};
      if (run.nodes != bench.expected) {
        std::cerr << std::format("{} d{}: got {}, expected {}\n", bench.name,
                                 bench.depth, run.nodes, bench.expected);
        return EXIT_FAILURE;
      }
      if (i == 0 || run.seconds < best.seconds) {
        best = run;
      }
    }

    total_nodes += best.nodes;
    total_seconds += best.seconds;
    std::cout << std::format(
        "{:<10} d{} {:>12} nodes {:>8.3f} s {:>12.0f} nps\n", bench.name,
        bench.depth, best.nodes, best.seconds,
        static_cast<double>(best.nodes) / best.seconds);
  }

  std::cout << std::format("Total: {} nodes {:.3f} s {:.0f} nps\n", total_nodes,
                           total_seconds,
                           static_cast<double>(total_nodes) / total_seconds);
  return EXIT_SUCCESS;
}
