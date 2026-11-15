#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <chess_ai/attacks.hpp>
#include <chess_ai/bitops.hpp>
#include <chess_ai/move.hpp>
#include <chess_ai/move_list.hpp>
#include <chess_ai/position.hpp>
#include <chess_ai/types.hpp>

// Generates all pseudo-legal moves (pawn, knight, bishop, rook, queen, king)
// for the side to move, appending them to `out`.
void generatePseudoLegalMoves(const Position& position, MoveList& out);

// Checks if a position is attacked by a color at a certain square.
auto isSquareAttacked(const Position& position, Square sq, Color by) -> bool;

// Checks if a color is in check.
auto inCheck(const Position& position, Color color) -> bool;

#endif  // !MOVEGEN_HPP
