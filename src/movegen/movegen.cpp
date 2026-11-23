#include <array>
#include <chess_ai/board/position.hpp>
#include <chess_ai/core/bitops.hpp>
#include <chess_ai/core/move.hpp>
#include <chess_ai/core/move_list.hpp>
#include <chess_ai/core/types.hpp>
#include <chess_ai/movegen/attacks.hpp>
#include <chess_ai/movegen/movegen.hpp>
#include <cstdint>

namespace {

// Tells what type of move generation to do.
enum class GenType : uint8_t { ALL, CAPTURES };

struct CastlingOption {
  CastlingRights right;
  BitBoard must_be_empty;  // every square between king and rook
  Square king_crosses;     // square the king passes over; must be unattacked
  Square king_to;
  MoveFlag flag;
};

struct CastlingSide {
  Square king_from;
  std::array<CastlingOption, 2> options;
};

constexpr BitBoard RANK_1{0x00000000000000FF};
constexpr BitBoard RANK_2{0x000000000000FF00};
constexpr BitBoard RANK_7{0x00FF000000000000};
constexpr BitBoard RANK_8{0xFF00000000000000};

// Returns the bitboard of squares one rank ahead of `bb`, from `color`'s
// perspective (toward the 8th rank for white, toward the 1st for black).
constexpr auto shiftForward(BitBoard bb, Color color) -> BitBoard {
  return color == Color::WHITE ? bb << 8U : bb >> 8U;
}

constexpr auto promotionRank(Color color) -> BitBoard {
  return color == Color::WHITE ? RANK_8 : RANK_1;
}

constexpr auto doublePushFromRank(Color color) -> BitBoard {
  return color == Color::WHITE ? RANK_2 : RANK_7;
}

// Rank from which a single push promotes.
constexpr auto prePromotionRank(Color color) -> BitBoard {
  return color == Color::WHITE ? RANK_7 : RANK_2;
}

// Indexed by `Color`.
constexpr std::array<CastlingSide, 2> CASTLING = {
    {
        {
            .king_from = Square::E1,
            .options =
                {
                    {
                        {
                            .right = CastlingRights::WHITE_KINGSIDE,
                            .must_be_empty = squareBitboard(Square::F1) |
                                             squareBitboard(Square::G1),
                            .king_crosses = Square::F1,
                            .king_to = Square::G1,
                            .flag = MoveFlag::CASTLE_KINGSIDE,
                        },
                        {
                            .right = CastlingRights::WHITE_QUEENSIDE,
                            .must_be_empty = squareBitboard(Square::B1) |
                                             squareBitboard(Square::C1) |
                                             squareBitboard(Square::D1),
                            .king_crosses = Square::D1,
                            .king_to = Square::C1,
                            .flag = MoveFlag::CASTLE_QUEENSIDE,
                        },
                    },
                },
        },
        {
            .king_from = Square::E8,
            .options =
                {
                    {
                        {
                            .right = CastlingRights::BLACK_KINGSIDE,
                            .must_be_empty = squareBitboard(Square::F8) |
                                             squareBitboard(Square::G8),
                            .king_crosses = Square::F8,
                            .king_to = Square::G8,
                            .flag = MoveFlag::CASTLE_KINGSIDE,
                        },
                        {
                            .right = CastlingRights::BLACK_QUEENSIDE,
                            .must_be_empty = squareBitboard(Square::B8) |
                                             squareBitboard(Square::C8) |
                                             squareBitboard(Square::D8),
                            .king_crosses = Square::D8,
                            .king_to = Square::C8,
                            .flag = MoveFlag::CASTLE_QUEENSIDE,
                        },
                    },
                },
        },
    },
};

template <GenType gen>
void genPawnPushes(const Position& position, Color color, MoveList& moves) {
  BitBoard pawns{position.pieces(color, PieceType::PAWN)};
  BitBoard empty{~position.occupiedAll()};
  BitBoard promo_rank{promotionRank(color)};

  BitBoard single_push_pawns{pawns};
  if constexpr (gen == GenType::CAPTURES) {
    single_push_pawns &= prePromotionRank(color);
  }
  while (single_push_pawns != 0) {
    Square from = popLSB(single_push_pawns);
    BitBoard from_bb{0};
    setBit(from_bb, from);
    BitBoard target{shiftForward(from_bb, color) & empty};
    if (target == 0) {
      continue;
    }
    Square to = getLSB(target);

    if ((target & promo_rank) != 0) {
      for (auto flag : {
               MoveFlag::PROMOTION_KNIGHT,
               MoveFlag::PROMOTION_BISHOP,
               MoveFlag::PROMOTION_ROOK,
               MoveFlag::PROMOTION_QUEEN,
           }) {
        moves.push(Move(from, to, flag));
      }
    } else {
      moves.push(Move(from, to, MoveFlag::QUIET));

      BitBoard start_rank_bb{0};
      setBit(start_rank_bb, from);
      if ((start_rank_bb & doublePushFromRank(color)) != 0) {
        BitBoard double_target{shiftForward(target, color) & empty};
        if (double_target != 0) {
          moves.push(Move(from, getLSB(double_target), MoveFlag::DOUBLE_PUSH));
        }
      }
    }
  }
}

void genPawnCaptures(const Position& position, Color color, MoveList& moves) {
  BitBoard pawns{position.pieces(color, PieceType::PAWN)};
  Color enemy = color == Color::WHITE ? Color::BLACK : Color::WHITE;
  BitBoard enemy_occupied{position.occupied(enemy)};
  BitBoard promo_rank{promotionRank(color)};
  Square ep_square = position.enPassantSquare();

  while (pawns != 0) {
    Square from = popLSB(pawns);
    BitBoard attacks{pawnAttacks(color, from)};

    BitBoard captures{attacks & enemy_occupied};
    while (captures != 0) {
      Square to = popLSB(captures);
      BitBoard to_bb{0};
      setBit(to_bb, to);
      if ((to_bb & promo_rank) != 0) {
        for (auto flag : {
                 MoveFlag::PROMOTION_CAPTURE_KNIGHT,
                 MoveFlag::PROMOTION_CAPTURE_BISHOP,
                 MoveFlag::PROMOTION_CAPTURE_ROOK,
                 MoveFlag::PROMOTION_CAPTURE_QUEEN,
             }) {
          moves.push(Move(from, to, flag));
        }
      } else {
        moves.push(Move(from, to, MoveFlag::CAPTURE));
      }
    }

    if (ep_square != Square::NONE && getBit(attacks, ep_square)) {
      moves.push(Move(from, ep_square, MoveFlag::EN_PASSANT));
    }
  }
}

template <PieceType type, auto attacksOf, GenType gen>
void generateLeapingMoves(const Position& position, MoveList& moves) {
  Color color = position.sideToMove();
  BitBoard own_occupied{position.occupied(color)};
  Color enemy = color == Color::WHITE ? Color::BLACK : Color::WHITE;
  BitBoard enemy_occupied{position.occupied(enemy)};

  BitBoard pieces{position.pieces(color, type)};
  while (pieces != 0) {
    Square from = popLSB(pieces);
    BitBoard targets{attacksOf(from) & ~own_occupied};

    if constexpr (gen == GenType::ALL) {
      BitBoard quiet_moves{targets & ~enemy_occupied};

      while (quiet_moves != 0) {
        moves.push(Move(from, popLSB(quiet_moves), MoveFlag::QUIET));
      }
    }

    BitBoard captures{targets & enemy_occupied};
    while (captures != 0) {
      moves.push(Move(from, popLSB(captures), MoveFlag::CAPTURE));
    }
  }
}

// Same shape as `generateLeapingMoves`, but `attacksOf` additionally takes
// the combined occupancy bitboard, since sliding-piece attacks depend on
// blockers along the ray rather than a fixed per-square table.
template <PieceType type, auto attacksOf, GenType gen>
void generateSlidingMoves(const Position& position, MoveList& moves) {
  Color color = position.sideToMove();
  BitBoard own_occupied{position.occupied(color)};
  Color enemy = color == Color::WHITE ? Color::BLACK : Color::WHITE;
  BitBoard enemy_occupied{position.occupied(enemy)};
  BitBoard occupied_all{position.occupiedAll()};

  BitBoard pieces{position.pieces(color, type)};
  while (pieces != 0) {
    Square from = popLSB(pieces);
    BitBoard targets{attacksOf(from, occupied_all) & ~own_occupied};

    if constexpr (gen == GenType::ALL) {
      BitBoard quiet_moves{targets & ~enemy_occupied};

      while (quiet_moves != 0) {
        moves.push(Move(from, popLSB(quiet_moves), MoveFlag::QUIET));
      }
    }

    BitBoard captures{targets & enemy_occupied};
    while (captures != 0) {
      moves.push(Move(from, popLSB(captures), MoveFlag::CAPTURE));
    }
  }
}

// Generates all pseudo-legal pawn moves (pushes, double pushes, captures,
// en passant, promotions) for the side to move, appending them to `moves`.
// In `GenType::CAPTURES` mode, pushes are limited to promotions.
template <GenType gen>
void generatePawnMoves(const Position& position, MoveList& moves) {
  Color color = position.sideToMove();
  genPawnPushes<gen>(position, color, moves);
  genPawnCaptures(position, color, moves);
}

void generateCastlingMoves(const Position& position, MoveList& moves) {
  const Color us = position.sideToMove();
  const Color them = opposite(us);
  const CastlingSide& side = CASTLING[static_cast<size_t>(us)];
  const uint8_t rights = position.castlingRightsBits();
  const BitBoard occ{position.occupiedAll()};

  // No rights, or king in check: cannot castle.
  if ((rights == 0) || isSquareAttacked(position, side.king_from, them)) {
    return;
  }

  for (const CastlingOption& option : side.options) {
    if ((rights & static_cast<uint8_t>(option.right)) != 0 &&
        (occ & option.must_be_empty) == 0 &&
        !isSquareAttacked(position, option.king_crosses, them)) {
      moves.push(Move(side.king_from, option.king_to, option.flag));
    }
  }
}

template <GenType gen>
void generateMoves(const Position& position, MoveList& out) {
  generatePawnMoves<gen>(position, out);
  generateLeapingMoves<PieceType::KNIGHT, knightAttacks, gen>(position, out);
  generateSlidingMoves<PieceType::BISHOP, bishopAttacks, gen>(position, out);
  generateSlidingMoves<PieceType::ROOK, rookAttacks, gen>(position, out);
  generateSlidingMoves<PieceType::QUEEN, queenAttacks, gen>(position, out);
  generateLeapingMoves<PieceType::KING, kingAttacks, gen>(position, out);
  if constexpr (gen == GenType::ALL) {
    generateCastlingMoves(position, out);
  }
}
}  // namespace

void generatePseudoLegalMoves(const Position& position, MoveList& out) {
  generateMoves<GenType::ALL>(position, out);
}

void generateCaptureMoves(const Position& position, MoveList& out) {
  generateMoves<GenType::CAPTURES>(position, out);
}

auto isSquareAttacked(const Position& position, Square sq, Color by) -> bool {
  const BitBoard occ{position.occupiedAll()};
  const BitBoard queens{position.pieces(by, PieceType::QUEEN)};
  const Color defender = opposite(by);

  if ((pawnAttacks(defender, sq) & position.pieces(by, PieceType::PAWN)) != 0) {
    return true;
  }
  if ((knightAttacks(sq) & position.pieces(by, PieceType::KNIGHT)) != 0) {
    return true;
  }
  if ((kingAttacks(sq) & position.pieces(by, PieceType::KING)) != 0) {
    return true;
  }
  if ((bishopAttacks(sq, occ) &
       (position.pieces(by, PieceType::BISHOP) | queens)) != 0) {
    return true;
  }
  if ((rookAttacks(sq, occ) &
       (position.pieces(by, PieceType::ROOK) | queens)) != 0) {
    return true;
  }
  return false;
}

auto inCheck(const Position& position, Color color) -> bool {
  const BitBoard king_bitboard{position.pieces(color, PieceType::KING)};
  const Square king_sq = getLSB(king_bitboard);

  return isSquareAttacked(position, king_sq, opposite(color));
}
