#include <chess_ai/uci/uci.hpp>
#include <iostream>

auto main() -> int {
  uciLoop(std::cin, std::cout);
  return 0;
}
