# Product

<!-- impeccable:product-schema 1 -->

## Platform

web

## Users

Primary: public club-level beginners, roughly under 1400, who have just finished a game
online. They arrive with a game they lost or did not understand, paste or import it, and
want to know what went wrong in words they can act on. They are strangers to the project
and owe it no patience.

Secondary (not a design driver, but real): the author, who uses the site as his own
coaching tool and as the visible half of a two-track learning project.

## Product Purpose

Chess Coach turns one of your own games into an explanation and then into practice. Paste
or import a PGN, step through the game on a board, see every move classified by how much
it cost (best / good / inaccuracy / mistake / blunder), read a plain-language explanation
of each mistake derived from the engine's own line, see the swing on an evaluation graph,
and then replay your own mistake positions as drills graded live. A second mode lets you
play a full timed game against the project's own engine at a measured strength level.

Success is a beginner who can name what they did wrong and recognise it next game.

## Positioning

Two things a neighboring analysis site cannot truthfully copy:

1. The explanations are derived from the engine's principal variation rather than from a
   library of hand-written tactic patterns, so the coaching follows the actual position.
2. The opponent you play is a chess engine written from scratch in C++ for this project
   and compiled to WebAssembly, with strength levels calibrated by a measured rating
   gauntlet rather than by an arbitrary slider.

## Operating Context

The usage scene is a player who just finished a game, most often on Lichess or Chess.com,
with the game still in mind. Games arrive three ways: pasted PGN, imported by username
from Lichess or Chess.com, or handed over from a game just played in the Play mode.

Sessions are remembered in localStorage, so returning to the site can resume the last
analysis. Analysis of a full game is a long-running job (every move goes to an engine in a
Web Worker), so progress and interruption are part of the real experience, not edge cases.

## Capabilities and Constraints

Shipped today:

- Play mode: a full timed game against the project's own WASM engine, choosing color, a
  strength level labeled with a measured Elo (plus a random-move Novice), and a time
  control with a real clock for both sides. Step back and forth, resign, or hand the
  finished game to analysis.
- Analyze mode: paste a PGN or import by username; per-move classification; beginner
  explanations; an eval bar beside the board; a chess.com-style evaluation graph with
  blunder markers and the best line at each point.
- Pattern detection across several games: mistakes broken down by phase
  (opening / middlegame / endgame) and category (dropped material / missed mate /
  missed capture / other), with a headline insight.
- Drills: your own mistake and blunder positions replayed as puzzles on a movable board,
  graded live, with under-promotion supported.

Durable product facts:

- The product is exactly two modes, Play and Analyze (analysis, patterns, and drills live
  under Analyze). This is the top-level structure.
- Explanations are written for a beginner and derived from the engine's own line. Jargon
  first, or a raw centipawn number as the primary answer, is wrong output.

Explicitly undecided (do not design as if settled):

- **Backend.** The app runs fully in the browser today and deploys as static assets on
  Cloudflare Workers, but the user has not committed to staying backendless. Treat
  "static, no server" as the current implementation, not a product constraint.
- **Which engine analyzes.** Stockfish is the analyst today because the project's own
  engine is not fast enough for depth-12 analysis, but the intent is for the project's
  engine to take that role as it matures. The two roles (analyst, opponent) are real; the
  identity of the analyst is not fixed.

## Brand Commitments

- Name: Chess Coach.
- Prose voice: no em dashes or en dashes anywhere in shipped copy.
- Beginner-legible language over chess-engine vocabulary.

## Evidence on Hand

Real and usable:

- A working C++ engine with a documented, tested milestone history (perft counts matching
  published references, A/B self-play Elo measurements for evaluation terms, a rating
  gauntlet behind the strength labels). See README.md and docs/superpowers/.
- The live deployment at https://chess-coach.aayus-pok.workers.dev.
- Real analysis output and drills produced by the app itself.

Absent, and not to be fabricated: users, testimonials, usage numbers, ratings improved,
press, pricing, or any third-party endorsement. The project has none.

## Product Principles

1. The game the visitor just played is the content. Get them from a pasted PGN to a real
   explanation with as little ceremony as possible.
2. Explain in beginner language, backed by the engine's line. An explanation a 1200 cannot
   act on has failed regardless of its accuracy.
3. Understanding is not the end; the drill is. Every mistake found should be practisable.
4. Analysis is slow and honest about it. Show progress, allow interruption, never pretend
   a long job is instant.
5. The from-scratch engine is a real part of the product, not a footnote. It is the
   opponent, and the work behind it is allowed to show.

## Accessibility & Inclusion

No formal standard committed. The floor is: keyboard usable, readable contrast, and the
site working on a phone without breaking.
