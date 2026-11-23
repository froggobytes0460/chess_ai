#ifndef CHESS_AI_MOVEGEN_MOVEGEN_HPP
#define CHESS_AI_MOVEGEN_MOVEGEN_HPP

#include <chess_ai/board/position.hpp>
#include <chess_ai/core/bitops.hpp>
#include <chess_ai/core/move.hpp>
#include <chess_ai/core/move_list.hpp>
#include <chess_ai/core/types.hpp>
#include <chess_ai/movegen/attacks.hpp>

// Generates all pseudo-legal moves (pawn, knight, bishop, rook, queen, king)
// for the side to move, appending them to `out`.
void generatePseudoLegalMoves(const Position& position, MoveList& out);

// Generates capture (and promotion) moves for the side to move, appending them
// to `out`.
void generateCaptureMoves(const Position& position, MoveList& out);

// Checks if a position is attacked by a color at a certain square.
auto isSquareAttacked(const Position& position, Square sq, Color by) -> bool;

// Checks if a color is in check.
auto inCheck(const Position& position, Color color) -> bool;

#endif  // !CHESS_AI_MOVEGEN_MOVEGEN_HPP
