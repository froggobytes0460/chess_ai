#include <array>
#include <chess_ai/attacks.hpp>
#include <chess_ai/bitops.hpp>
#include <chess_ai/move.hpp>
#include <chess_ai/move_list.hpp>
#include <chess_ai/movegen.hpp>
#include <chess_ai/position.hpp>
#include <chess_ai/types.hpp>

namespace {

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

constexpr std::array<MoveFlag, 4> PROMOTION_FLAGS = {
    MoveFlag::PROMOTION_KNIGHT,
    MoveFlag::PROMOTION_BISHOP,
    MoveFlag::PROMOTION_ROOK,
    MoveFlag::PROMOTION_QUEEN,
};

constexpr std::array<MoveFlag, 4> PROMOTION_CAPTURE_FLAGS = {
    MoveFlag::PROMOTION_CAPTURE_KNIGHT,
    MoveFlag::PROMOTION_CAPTURE_BISHOP,
    MoveFlag::PROMOTION_CAPTURE_ROOK,
    MoveFlag::PROMOTION_CAPTURE_QUEEN,
};

void genPawnPushes(const Position& position, Color color, MoveList& moves) {
  BitBoard pawns = position.pieces(color, PieceType::PAWN);
  BitBoard empty = ~position.occupiedAll();
  BitBoard promo_rank = promotionRank(color);

  BitBoard single_push_pawns = pawns;
  while (single_push_pawns != 0) {
    Square from = popLSB(single_push_pawns);
    BitBoard from_bb = 0;
    setBit(from_bb, from);
    BitBoard target = shiftForward(from_bb, color) & empty;
    if (target == 0) {
      continue;
    }
    Square to = getLSB(target);

    if ((target & promo_rank) != 0) {
      for (MoveFlag flag : PROMOTION_FLAGS) {
        moves.push(Move(from, to, flag));
      }
    } else {
      moves.push(Move(from, to, MoveFlag::QUIET));

      BitBoard start_rank_bb = 0;
      setBit(start_rank_bb, from);
      if ((start_rank_bb & doublePushFromRank(color)) != 0) {
        BitBoard double_target = shiftForward(target, color) & empty;
        if (double_target != 0) {
          moves.push(Move(from, getLSB(double_target), MoveFlag::DOUBLE_PUSH));
        }
      }
    }
  }
}

void genPawnCaptures(const Position& position, Color color, MoveList& moves) {
  BitBoard pawns = position.pieces(color, PieceType::PAWN);
  Color enemy = color == Color::WHITE ? Color::BLACK : Color::WHITE;
  BitBoard enemy_occupied = position.occupied(enemy);
  BitBoard promo_rank = promotionRank(color);
  Square ep_square = position.enPassantSquare();

  while (pawns != 0) {
    Square from = popLSB(pawns);
    BitBoard attacks = pawnAttacks(color, from);

    BitBoard captures = attacks & enemy_occupied;
    while (captures != 0) {
      Square to = popLSB(captures);
      BitBoard to_bb = 0;
      setBit(to_bb, to);
      if ((to_bb & promo_rank) != 0) {
        for (MoveFlag flag : PROMOTION_CAPTURE_FLAGS) {
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

template <PieceType type, auto attacksOf>
void generateLeapingMoves(const Position& position, MoveList& moves) {
  Color color = position.sideToMove();
  BitBoard own_occupied = position.occupied(color);
  Color enemy = color == Color::WHITE ? Color::BLACK : Color::WHITE;
  BitBoard enemy_occupied = position.occupied(enemy);

  BitBoard pieces = position.pieces(color, type);
  while (pieces != 0) {
    Square from = popLSB(pieces);
    BitBoard targets = attacksOf(from) & ~own_occupied;

    BitBoard quiet_moves = targets & ~enemy_occupied;
    while (quiet_moves != 0) {
      moves.push(Move(from, popLSB(quiet_moves), MoveFlag::QUIET));
    }

    BitBoard captures = targets & enemy_occupied;
    while (captures != 0) {
      moves.push(Move(from, popLSB(captures), MoveFlag::CAPTURE));
    }
  }
}

// Same shape as `generateLeapingMoves`, but `attacksOf` additionally takes
// the combined occupancy bitboard, since sliding-piece attacks depend on
// blockers along the ray rather than a fixed per-square table.
template <PieceType type, auto attacksOf>
void generateSlidingMoves(const Position& position, MoveList& moves) {
  Color color = position.sideToMove();
  BitBoard own_occupied = position.occupied(color);
  Color enemy = color == Color::WHITE ? Color::BLACK : Color::WHITE;
  BitBoard enemy_occupied = position.occupied(enemy);
  BitBoard occupied_all = position.occupiedAll();

  BitBoard pieces = position.pieces(color, type);
  while (pieces != 0) {
    Square from = popLSB(pieces);
    BitBoard targets = attacksOf(from, occupied_all) & ~own_occupied;

    BitBoard quiet_moves = targets & ~enemy_occupied;
    while (quiet_moves != 0) {
      moves.push(Move(from, popLSB(quiet_moves), MoveFlag::QUIET));
    }

    BitBoard captures = targets & enemy_occupied;
    while (captures != 0) {
      moves.push(Move(from, popLSB(captures), MoveFlag::CAPTURE));
    }
  }
}

// Generates all pseudo-legal pawn moves (pushes, double pushes, captures,
// en passant, promotions) for the side to move, appending them to `moves`.
void generatePawnMoves(const Position& position, MoveList& moves) {
  Color color = position.sideToMove();
  genPawnPushes(position, color, moves);
  genPawnCaptures(position, color, moves);
}

}  // namespace

void generatePseudoLegalMoves(const Position& position, MoveList& out) {
  generatePawnMoves(position, out);
  generateLeapingMoves<PieceType::KNIGHT, knightAttacks>(position, out);
  generateSlidingMoves<PieceType::BISHOP, bishopAttacks>(position, out);
  generateSlidingMoves<PieceType::ROOK, rookAttacks>(position, out);
  generateSlidingMoves<PieceType::QUEEN, queenAttacks>(position, out);
  generateLeapingMoves<PieceType::KING, kingAttacks>(position, out);
}
