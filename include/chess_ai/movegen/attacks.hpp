#ifndef CHESS_AI_MOVEGEN_ATTACKS_HPP
#define CHESS_AI_MOVEGEN_ATTACKS_HPP

// Public facade: pulls in leaping-piece (knight/king/pawn) and sliding-piece
// (rook/bishop/queen) attack lookups. Consumers should include this header
// rather than the split-out pieces directly, so the internal split can
// change without touching call sites.
#include <chess_ai/movegen/attacks_leaping.hpp>
#include <chess_ai/movegen/attacks_sliding.hpp>

#endif  // !CHESS_AI_MOVEGEN_ATTACKS_HPP
