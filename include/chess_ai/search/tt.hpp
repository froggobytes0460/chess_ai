#ifndef CHESS_AI_SEARCH_TT_HPP
#define CHESS_AI_SEARCH_TT_HPP

#include <chess_ai/core/move.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

enum class Bound : uint8_t { NONE, EXACT, LOWER, UPPER };

struct TTEntry {
  uint16_t key{};  // Verification bits
  Move move;
  int16_t score{};
  int8_t depth{};
  Bound bound{Bound::NONE};
};
static_assert(sizeof(TTEntry) == 8);

class TranspositionTable {
  std::vector<TTEntry> entries_;
  uint64_t mask_{};

 public:
  explicit TranspositionTable(size_t mb);

  // Reallocates to `mb` megabytes (rounded down to a power-of-two entry
  // count, at least one entry); all entries are lost.
  void resize(size_t mb);
  void clear();
  [[nodiscard]] auto probe(uint64_t key) const -> std::optional<TTEntry>;
  void store(uint64_t key, Move move, int32_t score, int depth, Bound bound);
};

#endif  // !CHESS_AI_SEARCH_TT_HPP
