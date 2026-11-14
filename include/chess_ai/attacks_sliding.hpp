#ifndef ATTACKS_SLIDING_HPP
#define ATTACKS_SLIDING_HPP

#include <array>
#include <chess_ai/bitops.hpp>
#include <chess_ai/types.hpp>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

// NOTE: These magic constants are generated from `magic_gen` tool.
constexpr std::array<uint64_t, 64> ROOK_MAGICS = {
    0x0080108000204000ULL, 0x00402000C0001000ULL, 0x0080100080200008ULL,
    0x0080080010000480ULL, 0x8200040200201008ULL, 0x0100180201000400ULL,
    0x2080010000800A00ULL, 0x008001000040A080ULL, 0x0200800080C00020ULL,
    0x0008402000401000ULL, 0x4040801000802008ULL, 0x0000801000800800ULL,
    0x0000800800800400ULL, 0x0000808004000200ULL, 0x0001000100020004ULL,
    0x0000800100004480ULL, 0x0080004020004000ULL, 0x0010004040002000ULL,
    0x0800808020001000ULL, 0x0000808050000800ULL, 0x0000808008000400ULL,
    0x0000808004000200ULL, 0x0003040010010208ULL, 0x0000020000640081ULL,
    0x5000800100204100ULL, 0x0000200040100840ULL, 0x0000100080802000ULL,
    0x0080201200420008ULL, 0x0004040080080080ULL, 0x0C00020080040080ULL,
    0x0001000100020004ULL, 0x0000010200204084ULL, 0x0000204000800082ULL,
    0x1100201000400040ULL, 0x4040801000802008ULL, 0x0000801000800800ULL,
    0x0000800800800400ULL, 0x0000804200800400ULL, 0x0100021104001028ULL,
    0x4002800040800100ULL, 0x0080004020004000ULL, 0x0010002000404000ULL,
    0x0000100020008080ULL, 0x0000080010008080ULL, 0x0000040008008080ULL,
    0x0000020004008080ULL, 0x0201000200030004ULL, 0x0000004100820004ULL,
    0x5000800100204100ULL, 0x0000200040100840ULL, 0x0000A00108401100ULL,
    0x0000080010008080ULL, 0x0004040080080080ULL, 0x0000020004008080ULL,
    0x0011001200040100ULL, 0x0000800100004480ULL, 0x0200800053004021ULL,
    0x0001001020400081ULL, 0x0000410008102001ULL, 0x0100100021000805ULL,
    0x8002001020080402ULL, 0x0001000400020801ULL, 0x0001001082000401ULL,
    0x0001000020804201ULL,
};
constexpr std::array<uint64_t, 64> BISHOP_MAGICS = {
    0x0020440122040210ULL, 0x8004100200410000ULL, 0x0004040082000084ULL,
    0x0008060840000000ULL, 0x0003104000000000ULL, 0x1002482004000100ULL,
    0x0001410820100200ULL, 0x0020402084104004ULL, 0x800008200C040040ULL,
    0x0000200809004080ULL, 0x0000118404004001ULL, 0x4000044040800000ULL,
    0x0000040420000000ULL, 0x0001820802080000ULL, 0x00004400C8080800ULL,
    0x0840420101411000ULL, 0x0804001020020408ULL, 0x0002001010014502ULL,
    0x0001000208010100ULL, 0x10040440C4008000ULL, 0x1001000090400000ULL,
    0x0000800108200200ULL, 0x0000400088080800ULL, 0x00010000A4013404ULL,
    0x0020080004080880ULL, 0x0090040006040420ULL, 0x0101100021040020ULL,
    0x0000802088020220ULL, 0x0001001003014000ULL, 0x0018020020410080ULL,
    0x0002004004050800ULL, 0x0001004200220800ULL, 0x0004200800600200ULL,
    0x0001082000480108ULL, 0x0040203000080880ULL, 0x0000020080080080ULL,
    0x0004002400020108ULL, 0x0001010200010040ULL, 0x2004008180040400ULL,
    0x0081004100028400ULL, 0x4008020820800404ULL, 0x0000841008000280ULL,
    0x08104202C1001000ULL, 0x0100202218000400ULL, 0x000004010A000400ULL,
    0x00100A10040800C0ULL, 0x0020010202000080ULL, 0x0022080441800300ULL,
    0x0001410820100200ULL, 0x0000840C09040000ULL, 0x000082004A080080ULL,
    0x0000000020880000ULL, 0x0000803002120000ULL, 0x0008202002008800ULL,
    0x0004200801050000ULL, 0x8004100200410000ULL, 0x0020402084104004ULL,
    0x0840420101411000ULL, 0x0808000042009000ULL, 0x0000002003084800ULL,
    0x0000000010020200ULL, 0x0000001020010100ULL, 0x800008200C040040ULL,
    0x0020440122040210ULL,
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

  BitBoard subset = 0;
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
  BitBoard relevant = occupied & table.mask;
  size_t idx =
      (relevant * ROOK_MAGICS[static_cast<unsigned>(sq)]) >> table.shift;
  return table.attacks[idx];
}

// Returns the bishop attack bitboard for `sq` given the current `occupied`
// bitboard, via magic bitboard lookup.
[[nodiscard]] inline auto bishopAttacks(Square sq, BitBoard occupied)
    -> BitBoard {
  const auto& table = bishop_tables[static_cast<unsigned>(sq)];
  BitBoard relevant = occupied & table.mask;
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

#endif  // !ATTACKS_SLIDING_HPP
