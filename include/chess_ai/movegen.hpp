#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <chess_ai/move_list.hpp>
#include <chess_ai/position.hpp>

// Generates all pseudo-legal pawn moves (pushes, double pushes, captures,
// en passant, promotions) for the side to move, appending them to `moves`.
void generatePawnMoves(const Position& position, MoveList& moves);

#endif  // !MOVEGEN_HPP
