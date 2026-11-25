// NOLINTBEGIN(performance-avoid-endl)
#include <algorithm>
#include <atomic>
#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move.hpp>
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
  std::string token;

  while (args >> token) {
    if (token == "depth") {
      int d{};
      args >> d;
      limits.depth = std::clamp(d, 1, MAX_DEPTH);
    } else if (token == "nodes") {
      args >> limits.nodes;
    } else if (token == "movetime") {
      int64_t ms{};
      args >> ms;
      limits.deadline =
          std::chrono::steady_clock::now() + std::chrono::milliseconds{ms};
    }
    // TODO: Implement wtime/btime/winc/binc/movestogo/infinite later.
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
