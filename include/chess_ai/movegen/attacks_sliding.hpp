#ifndef CHESS_AI_MOVEGEN_ATTACKS_SLIDING_HPP
#define CHESS_AI_MOVEGEN_ATTACKS_SLIDING_HPP

#include <array>
#include <chess_ai/core/bitops.hpp>
#include <chess_ai/core/types.hpp>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

// NOTE: These magic constants are generated from `magic_gen` tool.
constexpr std::array<uint64_t, 64> ROOK_MAGICS{
    0x0080108000204000, 0x00402000C0001000, 0x0080100080200008,
    0x0080080010000480, 0x8200040200201008, 0x0100180201000400,
    0x2080010000800A00, 0x008001000040A080, 0x0200800080C00020,
    0x0008402000401000, 0x4040801000802008, 0x0000801000800800,
    0x0000800800800400, 0x0000808004000200, 0x0001000100020004,
    0x0000800100004480, 0x0080004020004000, 0x0010004040002000,
    0x0800808020001000, 0x0000808050000800, 0x0000808008000400,
    0x0000808004000200, 0x0003040010010208, 0x0000020000640081,
    0x5000800100204100, 0x0000200040100840, 0x0000100080802000,
    0x0080201200420008, 0x0004040080080080, 0x0C00020080040080,
    0x0001000100020004, 0x0000010200204084, 0x0000204000800082,
    0x1100201000400040, 0x4040801000802008, 0x0000801000800800,
    0x0000800800800400, 0x0000804200800400, 0x0100021104001028,
    0x4002800040800100, 0x0080004020004000, 0x0010002000404000,
    0x0000100020008080, 0x0000080010008080, 0x0000040008008080,
    0x0000020004008080, 0x0201000200030004, 0x0000004100820004,
    0x5000800100204100, 0x0000200040100840, 0x0000A00108401100,
    0x0000080010008080, 0x0004040080080080, 0x0000020004008080,
    0x0011001200040100, 0x0000800100004480, 0x0200800053004021,
    0x0001001020400081, 0x0000410008102001, 0x0100100021000805,
    0x8002001020080402, 0x0001000400020801, 0x0001001082000401,
    0x0001000020804201,
};
constexpr std::array<uint64_t, 64> BISHOP_MAGICS{
    0x0020440122040210, 0x8004100200410000, 0x0004040082000084,
    0x0008060840000000, 0x0003104000000000, 0x1002482004000100,
    0x0001410820100200, 0x0020402084104004, 0x800008200C040040,
    0x0000200809004080, 0x0000118404004001, 0x4000044040800000,
    0x0000040420000000, 0x0001820802080000, 0x00004400C8080800,
    0x0840420101411000, 0x0804001020020408, 0x0002001010014502,
    0x0001000208010100, 0x10040440C4008000, 0x1001000090400000,
    0x0000800108200200, 0x0000400088080800, 0x00010000A4013404,
    0x0020080004080880, 0x0090040006040420, 0x0101100021040020,
    0x0000802088020220, 0x0001001003014000, 0x0018020020410080,
    0x0002004004050800, 0x0001004200220800, 0x0004200800600200,
    0x0001082000480108, 0x0040203000080880, 0x0000020080080080,
    0x0004002400020108, 0x0001010200010040, 0x2004008180040400,
    0x0081004100028400, 0x4008020820800404, 0x0000841008000280,
    0x08104202C1001000, 0x0100202218000400, 0x000004010A000400,
    0x00100A10040800C0, 0x0020010202000080, 0x0022080441800300,
    0x0001410820100200, 0x0000840C09040000, 0x000082004A080080,
    0x0000000020880000, 0x0000803002120000, 0x0008202002008800,
    0x0004200801050000, 0x8004100200410000, 0x0020402084104004,
    0x0840420101411000, 0x0808000042009000, 0x0000002003084800,
    0x0000000010020200, 0x0000001020010100, 0x800008200C040040,
    0x0020440122040210,
};

namespace detail {

// Direction deltas: rook rays (N/S/E/W), bishop rays (NE/NW/SE/SW).
constexpr std::array<std::pair<int, int>, 4> ROOK_DIRS = {
    {{0, 1}, {0, -1}, {1, 0}, {-1, 0}},
};
constexpr std::array<std::pair<int, int>, 4> BISHOP_DIRS = {
    {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}},
};

// Builds the "relevant occupancy" mask for a sliding piece on `sq`: every
// square it rays through in `dirs`, excluding the board edge (a blocker on
// the edge can't block anything further, so leaving it out keeps the mask,
// and therefore the magic lookup table, as small as possible).
inline auto slidingMask(Square sq,
                        const std::array<std::pair<int, int>, 4>& dirs)
    -> BitBoard {
  BitBoard mask{};
  int file = static_cast<int>(sq) % 8;
  int rank = static_cast<int>(sq) / 8;
  for (const auto& [delta_file, delta_rank] : dirs) {
    int target_file = file + delta_file;
    int target_rank = rank + delta_rank;
    while (target_file + delta_file >= 0 && target_file + delta_file <= 7 &&
           target_rank + delta_rank >= 0 && target_rank + delta_rank <= 7) {
      setBit(mask, static_cast<Square>((target_rank * 8) + target_file));
      target_file += delta_file;
      target_rank += delta_rank;
    }
  }
  return mask;
}

// Brute-force ray-cast attacks for a sliding piece on `sq` given a concrete
// `occupied` bitboard. Used only to build the magic lookup tables once at
// startup, never on the hot path.
inline auto slidingAttacksSlow(Square sq, BitBoard occupied,
                               const std::array<std::pair<int, int>, 4>& dirs)
    -> BitBoard {
  BitBoard attacks{};
  int file = static_cast<int>(sq) % 8;
  int rank = static_cast<int>(sq) / 8;
  for (const auto& [delta_file, delta_rank] : dirs) {
    int target_file = file + delta_file;
    int target_rank = rank + delta_rank;
    while (target_file >= 0 && target_file <= 7 && target_rank >= 0 &&
           target_rank <= 7) {
      auto target = static_cast<Square>((target_rank * 8) + target_file);
      setBit(attacks, target);
      if (getBit(occupied, target)) {
        break;
      }
      target_file += delta_file;
      target_rank += delta_rank;
    }
  }
  return attacks;
}

// Per-square magic lookup state: the relevant-occupancy mask, the shift used
// to turn `(occupancy & mask) * magic` into a table index, and the
// precomputed attack table itself (sized `1 << popCount(mask)`).
struct SlidingTable {
  BitBoard mask{};
  int shift{};
  std::vector<BitBoard> attacks;
};

// Builds the magic lookup table for one square: enumerates every subset of
// `mask` (Carry-Rippler trick) and stores the brute-force attack result at
// the index the magic number maps it to.
inline auto buildSlidingTable(Square sq, uint64_t magic,
                              const std::array<std::pair<int, int>, 4>& dirs)
    -> SlidingTable {
  SlidingTable table;
  table.mask = slidingMask(sq, dirs);
  int bits = popCount(table.mask);
  table.shift = 64 - bits;
  table.attacks.resize(uint64_t{1} << bits);

  BitBoard subset{0};
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while)
  do {
    size_t idx = (subset * magic) >> table.shift;
    table.attacks[idx] = slidingAttacksSlow(sq, subset, dirs);
    subset = (subset - table.mask) & table.mask;
  } while (subset != 0);

  return table;
}

template <const std::array<uint64_t, 64>& magics,
          const std::array<std::pair<int, int>, 4>& dirs>
auto buildAllSlidingTables() -> std::array<SlidingTable, 64> {
  std::array<SlidingTable, 64> tables;
  for (int sq = 0; sq < 64; ++sq) {
    // NOLINTNEXTLINE(readability-redundant-parentheses)
    tables[sq] = buildSlidingTable(static_cast<Square>(sq), magics[sq], dirs);
  }
  return tables;
}

}  // namespace detail

namespace {
// OOM at startup is fine for this toy engine; there is nothing meaningful
// to catch it with.
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
const std::array<detail::SlidingTable, 64> rook_tables =
    detail::buildAllSlidingTables<ROOK_MAGICS, detail::ROOK_DIRS>();
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
const std::array<detail::SlidingTable, 64> bishop_tables =
    detail::buildAllSlidingTables<BISHOP_MAGICS, detail::BISHOP_DIRS>();
}  // namespace

// Returns the rook attack bitboard for `sq` given the current `occupied`
// bitboard, via magic bitboard lookup.
[[nodiscard]] inline auto rookAttacks(Square sq, BitBoard occupied)
    -> BitBoard {
  const auto& table = rook_tables[static_cast<unsigned>(sq)];
  BitBoard relevant{occupied & table.mask};
  size_t idx =
      (relevant * ROOK_MAGICS[static_cast<unsigned>(sq)]) >> table.shift;
  return table.attacks[idx];
}

// Returns the bishop attack bitboard for `sq` given the current `occupied`
// bitboard, via magic bitboard lookup.
[[nodiscard]] inline auto bishopAttacks(Square sq, BitBoard occupied)
    -> BitBoard {
  const auto& table = bishop_tables[static_cast<unsigned>(sq)];
  BitBoard relevant{occupied & table.mask};
  size_t idx =
      (relevant * BISHOP_MAGICS[static_cast<unsigned>(sq)]) >> table.shift;
  return table.attacks[idx];
}

// Returns the queen attack bitboard for `sq`: the union of rook and bishop
// attacks from the same square.
[[nodiscard]] inline auto queenAttacks(Square sq, BitBoard occupied)
    -> BitBoard {
  return rookAttacks(sq, occupied) | bishopAttacks(sq, occupied);
}

#endif  // !CHESS_AI_MOVEGEN_ATTACKS_SLIDING_HPP
