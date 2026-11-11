#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <chess_ai/attacks.hpp>
#include <chess_ai/bitops.hpp>
#include <chess_ai/move.hpp>
#include <chess_ai/move_list.hpp>
#include <chess_ai/position.hpp>
#include <chess_ai/types.hpp>

// Generates all pseudo-legal pawn moves (pushes, double pushes, captures,
// en passant, promotions) for the side to move, appending them to `moves`.
void generatePawnMoves(const Position& position, MoveList& moves);

// Generates all pseudo-legal knight moves for the side to move.
void generateKnightMoves(const Position& position, MoveList& moves);

// Generates all pseudo-legal king moves for the side to move (castling is
// generated separately; this covers single-step king moves only).
void generateKingMoves(const Position& position, MoveList& moves);

void generatePseudoLegalMoves(const Position& position, MoveList& out);

#endif  // !MOVEGEN_HPP
