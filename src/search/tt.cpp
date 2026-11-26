#include <algorithm>
#include <bit>
#include <chess_ai/core/move.hpp>
#include <chess_ai/search/tt.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace {
constexpr int KEY_SHIFT{48};

constexpr auto verifyKey(uint64_t key) -> uint16_t {
  return static_cast<uint16_t>(key >> KEY_SHIFT);
}
}  // namespace

TranspositionTable::TranspositionTable(size_t mb) { resize(mb); }

void TranspositionTable::resize(size_t mb) {
  // MB << 20 == Bytes
  const size_t count =
      std::max<size_t>(std::bit_floor((mb << 20) / sizeof(TTEntry)), 1);
  // Fresh vector, so shrinking releases memory too.
  entries_ = std::vector<TTEntry>(count);
  mask_ = count - 1;
}

void TranspositionTable::clear() { std::ranges::fill(entries_, TTEntry{}); }

auto TranspositionTable::probe(uint64_t key) const -> std::optional<TTEntry> {
  const TTEntry& entry = entries_[key & mask_];
  if (entry.bound == Bound::NONE || entry.key != verifyKey(key)) {
    return std::nullopt;
  }
  return entry;
}

void TranspositionTable::store(uint64_t key, Move move, int32_t score,
                               int depth, Bound bound) {
  TTEntry& entry = entries_[key & mask_];
  const uint16_t verify{verifyKey(key)};
  if (move != Move{} || entry.key != verify) {
    entry.move = move;
  }
  entry.key = verify;
  entry.score = static_cast<int16_t>(score);
  entry.depth = static_cast<int8_t>(depth);
  entry.bound = bound;
}
