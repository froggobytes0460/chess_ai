#!/usr/bin/env bash
# SPRT match: current working tree (dev) vs a baseline git ref (base).
#
# Usage: scripts/sprt.sh [-m gain|nonreg] [-t tc] [-c concurrency]
#                        [-o openings.pgn] [base_ref]
#
#   base_ref  commit/branch to test against (default: main)
#   -m gain   strength change, bounds elo0=0 elo1=10 (default)
#   -m nonreg refactor/speedup, must not lose: elo0=-10 elo1=0
#   -t        time control (default: 10+0.1)
#   -c        concurrent games (default: nproc / 2)
#   -o        opening book in PGN format (default: openings.pgn in repo root;
#             if missing, Stockfish's 8moves_v3.pgn is downloaded there)
#
# Both engines are built with the release preset and copied to build/sprt/,
# so rebuilding during a run does not swap an engine mid-match. The baseline
# is built in a temporary git worktree; the working tree is never touched.
# Results: fastchess prints the SPRT verdict; games go to build/sprt/*.pgn.

set -euo pipefail

mode=gain
tc=10+0.1
concurrency=$(($(nproc) / 2))
repo=$(git rev-parse --show-toplevel)
openings=$repo/openings.pgn
# Fallback book if openings.pgn is missing: Stockfish's 8moves_v3, pinned.
BOOK_URL=https://github.com/official-stockfish/books/raw/master/8moves_v3.pgn.zip
BOOK_SHA256=5835239f88cc2c7511b177c32392a69f3ede21819cf0616f80a7f907cd21d17e

usage() {
  sed -n '2,18s/^# \{0,1\}//p' "$0"
  exit 1
}

while getopts "m:t:c:o:h" opt; do
  case $opt in
  m) mode=$OPTARG ;;
  t) tc=$OPTARG ;;
  c) concurrency=$OPTARG ;;
  o) openings=$(realpath "$OPTARG") ;;
  *) usage ;;
  esac
done
shift $((OPTIND - 1))
base_ref=${1:-main}

case $mode in
gain) bounds="elo0=0 elo1=10" ;;
nonreg) bounds="elo0=-10 elo1=0" ;;
*)
  echo "error: unknown mode '$mode' (gain|nonreg)" >&2
  exit 1
  ;;
esac

if [[ ! -f $openings && $openings == "$repo/openings.pgn" ]]; then
  echo "== $openings missing, downloading $BOOK_URL"
  book_tmp=$(mktemp -d "${TMPDIR:-/tmp}/chess_ai_book.XXXXXX")
  curl -fsSL -o "$book_tmp/book.zip" "$BOOK_URL"
  unzip -p "$book_tmp/book.zip" 8moves_v3.pgn >"$book_tmp/book.pgn"
  sha=$(sha256sum "$book_tmp/book.pgn" | cut -d' ' -f1)
  if [[ $sha != "$BOOK_SHA256" ]]; then
    echo "error: downloaded book sha256 $sha != expected $BOOK_SHA256" >&2
    rm -rf "$book_tmp"
    exit 1
  fi
  mv "$book_tmp/book.pgn" "$openings"
  rm -rf "$book_tmp"
fi
[[ -f $openings ]] || {
  echo "error: opening book '$openings' not found" >&2
  exit 1
}
command -v fastchess >/dev/null || {
  echo "error: fastchess not in PATH" >&2
  exit 1
}

base_sha=$(git -C "$repo" rev-parse --verify --short "$base_ref^{commit}")
dev_sha=$(git -C "$repo" rev-parse --short HEAD)
if [[ -n $(git -C "$repo" status --porcelain --untracked-files=no) ]]; then
  dev_sha+=-dirty
fi

out_dir=$repo/build/sprt
mkdir -p "$out_dir"
worktree=$(mktemp -d "${TMPDIR:-/tmp}/chess_ai_base.XXXXXX")

cleanup() {
  git -C "$repo" worktree remove --force "$worktree" 2>/dev/null || rm -rf "$worktree"
}
trap cleanup EXIT
trap 'exit 130' INT TERM # Ctrl-C mid-match still removes the worktree

echo "== Building dev ($dev_sha)"
(cd "$repo" && cmake --preset release >/dev/null && cmake --build --preset release)
cp "$repo/build/release/main" "$out_dir/dev"

echo "== Building base ($base_ref = $base_sha)"
git -C "$repo" worktree add --detach --quiet "$worktree" "$base_sha"
(cd "$worktree" && cmake --preset release >/dev/null && cmake --build --preset release)
cp "$worktree/build/release/main" "$out_dir/base"

pgn=$out_dir/${dev_sha}_vs_${base_sha}_$(date +%Y%m%d-%H%M%S).pgn

echo "== SPRT [$mode: $bounds] tc=$tc concurrency=$concurrency"
# Run from out_dir so fastchess's config.json lands there, not in the repo.
cd "$out_dir"
# shellcheck disable=SC2086  # $bounds is two words on purpose
fastchess \
  -engine cmd="$out_dir/dev" name="dev-$dev_sha" \
  -engine cmd="$out_dir/base" name="base-$base_sha" \
  -each tc="$tc" \
  -openings file="$openings" format=pgn order=random \
  -rounds 5000 -games 2 -repeat -concurrency "$concurrency" \
  -sprt $bounds alpha=0.05 beta=0.05 \
  -pgnout file="$pgn"

echo "== Games: $pgn"
