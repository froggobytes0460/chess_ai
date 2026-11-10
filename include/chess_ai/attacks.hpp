#ifndef ATTACKS_HPP
#define ATTACKS_HPP

// Public facade: pulls in leaping-piece (knight/king/pawn) and sliding-piece
// (rook/bishop/queen) attack lookups. Consumers should include this header
// rather than the split-out pieces directly, so the internal split can
// change without touching call sites.
#include <chess_ai/attacks_leaping.hpp>
#include <chess_ai/attacks_sliding.hpp>

#endif  // !ATTACKS_HPP
