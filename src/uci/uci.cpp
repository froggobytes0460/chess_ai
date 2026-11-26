// NOLINTBEGIN(performance-avoid-endl)
#include <algorithm>
#include <atomic>
#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move.hpp>
#include <chess_ai/core/types.hpp>
#include <chess_ai/search/search.hpp>
#include <chess_ai/uci/move_conv.hpp>
#include <chess_ai/uci/uci.hpp>
#include <chrono>
#include <cstdint>
#include <format>
#include <istream>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {
constexpr std::string_view START_POS{
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"};
constexpr int64_t MOVE_OVERHEAD_MS{50};
constexpr int64_t DEFAULT_MOVES_TO_GO{30};
struct Engine {
  Position pos{START_POS};
  std::atomic<bool> stop{false};
};

void handlePosition(Engine& eng, std::istringstream& args) {
  std::string token;
  args >> token;

  std::string fen;
  if (token == "startpos") {
    fen = START_POS;
    args >> token;
  } else if (token == "fen") {
    while (args >> token && token != "moves") {
      if (!fen.empty()) {
        fen += ' ';
      }
      fen += token;
    }
  } else {
    return;
  }

  eng.pos = Position{fen};

  if (token != "moves") {
    return;
  }
  while (args >> token) {
    const std::optional<Move> move{uciToMove(eng.pos, token)};
    if (!move) {
      break;
    }
    static_cast<void>(eng.pos.makeMove(*move));
  }
}
void handleGo(Engine& eng, std::istringstream& args, std::ostream& out) {
  SearchLimits limits{};
  std::optional<int64_t> wtime;
  std::optional<int64_t> btime;
  std::optional<int64_t> movetime;
  int64_t winc{0};
  int64_t binc{0};
  int64_t movestogo{0};
  std::string token;

  while (args >> token) {
    if (token == "depth") {
      int d{};
      args >> d;
      limits.depth = std::clamp(d, 1, MAX_DEPTH);
    } else if (token == "nodes") {
      args >> limits.nodes;
    } else if (token == "movetime") {
      movetime.emplace();
      args >> *movetime;
    } else if (token == "wtime") {
      wtime.emplace();
      args >> *wtime;
    } else if (token == "btime") {
      btime.emplace();
      args >> *btime;
    } else if (token == "winc") {
      args >> winc;
    } else if (token == "binc") {
      args >> binc;
    } else if (token == "movestogo") {
      args >> movestogo;
    }
  }

  const bool is_white{eng.pos.sideToMove() == Color::WHITE};
  const std::optional<int64_t> time{is_white ? wtime : btime};
  const int64_t inc{is_white ? winc : binc};

  const auto start{std::chrono::steady_clock::now()};
  if (movetime.has_value()) {
    limits.deadline = start + std::chrono::milliseconds(*movetime);
  } else if (time.has_value()) {
    const int64_t mtg{movestogo > 0 ? movestogo : DEFAULT_MOVES_TO_GO};
    int64_t budget{(*time / mtg) + (inc / 2)};
    budget = std::min(budget, *time - MOVE_OVERHEAD_MS);
    budget = std::max<int64_t>(budget, 1);
    limits.deadline = start + std::chrono::milliseconds(budget);
  }

  Position copy{eng.pos};
  eng.stop = false;
  const SearchResult res{search(copy, limits, eng.stop)};

  out << std::format("info depth {} score cp {} nodes {}\n", res.depth,
                     res.score, res.nodes)
      << std::format("bestmove {}", res.best_move == Move{}
                                        ? "0000"
                                        : moveToUci(res.best_move))
      << std::endl;
}
}  // namespace

void uciLoop(std::istream& in_stream, std::ostream& out_stream) {
  Engine eng;
  std::string line;

  while (std::getline(in_stream, line)) {
    std::istringstream args{line};
    std::string cmd;
    args >> cmd;

    if (cmd == "uci") {
      out_stream << "id name chess_ai\n"
                 << "id author Yasir Atiq Mohammed\n"
                 << "uciok" << std::endl;
    } else if (cmd == "isready") {
      out_stream << "readyok" << std::endl;
    } else if (cmd == "ucinewgame") {
      eng.pos = Position{START_POS};
    } else if (cmd == "position") {
      handlePosition(eng, args);
    } else if (cmd == "go") {
      handleGo(eng, args, out_stream);
    } else if (cmd == "quit") {
      break;
    }
  }
}
// NOLINTEND(performance-avoid-endl)
