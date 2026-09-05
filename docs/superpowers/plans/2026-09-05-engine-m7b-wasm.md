# Engine M7b: Run Our Engine in the Coach (WASM) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Compile our C++ engine to WebAssembly and run it in the coach's Web Worker in place of Stockfish, and give its UCI output the `info ... score ... pv ...` line the coach needs to classify and explain moves.

**Architecture:** Phase 1 (unblocked) is pure C++: the search returns a principal variation and depth, and `handle_go` emits a rich `info` line (score cp/mate, nodes, nps, time, pv) before `bestmove`. Phase 2 (needs Emscripten) adds a tiny C entry `uci_command(const char*)` that drives the existing `handle_command`, compiles the `engine` library to WASM via an `if(EMSCRIPTEN)` branch in the existing CMake, wraps it in a Worker script that mirrors Stockfish's message contract, and points the coach's `ENGINE_URL` at it.

**Tech Stack:** C++17 (MSYS2 UCRT64, gcc) + doctest for Phase 1; Emscripten (emsdk) + Vite/TS for Phase 2. Build/test via PowerShell.

**Spec:** inline (design approved in chat 2026-09-05).

## Global Constraints

- The coach's `Engine` (web/src/engine.ts:11) drives a Worker with UCI strings: `uci`->`uciok`, `position fen <FEN>`, `go depth N`, and reads `info ... score cp|mate ... pv ...` (parseInfo) plus `bestmove` (parseBestMove). Match that contract exactly.
- `position fen` and `go depth N` already work (uci.cpp:121, compute_limits uci.cpp:74). The missing piece is the `info` line with score.
- Score is side-to-move perspective (same convention as Stockfish and as `evaluate`).
- `pvBefore` is currently stored but unread by the coach; a correct single-line PV is enough. multipv and per-depth `info` streaming are OUT of scope (nothing consumes them); if added later, flag with a `ponytail:` note.
- Reuse `piece_value`, `to_uci`, `nodes_searched()`; do not duplicate.
- No dashes in prose, comments, commits.
- Native build loop (PowerShell, PATH refreshed):
  `$env:Path = [Environment]::GetEnvironmentVariable("Path","User") + ";" + [Environment]::GetEnvironmentVariable("Path","Machine")`
  then `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`

---

## Phase 1: UCI info line + principal variation (unblocked, C++)

### Task 1: Principal variation and reached depth in the search

**Files:**
- Modify: `engine/src/search.hpp` (add `pv`, `depth` to `SearchResult`)
- Modify: `engine/src/search.cpp` (triangular PV table in `negamax`; fill in `search_to_depth`/`search`/`search_timed`)
- Test: `engine/tests/test_search.cpp`

**Interfaces:**
- Produces: `SearchResult { Move best; int score; std::vector<Move> pv; int depth; }`. `pv[0] == best` whenever a legal move exists; `pv` is the main search line (it stops at the quiescence horizon). `depth` is the last fully completed search depth.

- [ ] **Step 1: Write the failing test**

```cpp
TEST_CASE("search returns a principal variation starting with the best move") {
    Board b = board_from_fen("4k3/8/8/8/8/3q4/8/3RK3 w - - 0 1"); // Rd1xd3 free queen
    SearchResult r = search(b, 3);
    REQUIRE(r.pv.size() >= 1);
    CHECK(r.pv[0].from == r.best.from);
    CHECK(r.pv[0].to   == r.best.to);
    CHECK(r.depth == 3);
}
```

- [ ] **Step 2: Run it, verify it fails to compile (no `pv`/`depth` member)**

Run: `cmake --build engine/build`
Expected: compile error, `pv`/`depth` not a member of SearchResult.

- [ ] **Step 3: Add the fields to `SearchResult` in search.hpp**

```cpp
struct SearchResult {
    Move best;               // best move found (best.from == NO_SQUARE if none)
    int score;               // centipawns, side-to-move perspective
    std::vector<Move> pv;    // principal variation, pv[0] == best
    int depth = 0;           // last fully completed depth
};
```
Add `#include <vector>` to search.hpp.

- [ ] **Step 4: Add a triangular PV table to `negamax`**

At file scope in the anon namespace (near the other globals):

```cpp
const int MAXPLY = 128;
Move g_pv[MAXPLY][MAXPLY];
int  g_pv_len[MAXPLY];
```

In `negamax`, set the line length to zero on entry (after `g_nodes++`):

```cpp
    g_pv_len[ply] = 0;
```

When a move becomes the new best AND raises alpha (a PV node), record it and splice the child's PV. Replace the update block inside the move loop:

```cpp
        if (score > best) {
            best = score;
            if (score > alpha) {
                alpha = score;
                if (ply + 1 < MAXPLY) {
                    g_pv[ply][0] = m;
                    int n = g_pv_len[ply + 1];
                    for (int i = 0; i < n && i + 1 < MAXPLY; i++)
                        g_pv[ply][i + 1] = g_pv[ply + 1][i];
                    g_pv_len[ply] = n + 1;
                }
            }
        }
        if (alpha >= beta) break;   // beta cutoff
```

(The old code folded `best`/`alpha` updates together; this splits them so the PV is only recorded on an alpha improvement. `quiesce` and `negamax_full` are unchanged: the PV stops at the main-search horizon, which is what the coach wants.)

- [ ] **Step 5: Collect the PV at the root in `search_to_depth`**

After the root move loop sets `result.best`/`result.score`, build the PV from the best root move plus the child line. Simplest correct approach: when a root move becomes the new best, capture the line the same way.

Inside the root loop, when `score > best`:

```cpp
        if (score > best) {
            best = score;
            result.best = m;
            result.pv.clear();
            result.pv.push_back(m);
            for (int i = 0; i < g_pv_len[1] && i < MAXPLY; i++)
                result.pv.push_back(g_pv[1][i]);
        }
```

(`g_pv[1]` is the line below the root move, filled by the `negamax(..., ply=1, ...)` call.) Set `result.depth = depth;` before returning.

- [ ] **Step 6: Carry pv/depth through `search` and `search_timed`**

`search` and `search_timed` already assign `result = search_to_depth(...)`, so `pv`/`depth` propagate for free. Confirm `search_timed` keeps the last completed depth's result (it does: `best = r` only on a completed depth), so `best.pv` and `best.depth` are the last good line.

- [ ] **Step 7: Build and run the full suite**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: ALL PASS, including the new PV test and every prior test (PV bookkeeping does not change any score, so the differential and ID tests still hold).

- [ ] **Step 8: Commit**

```bash
git add engine/src/search.hpp engine/src/search.cpp engine/tests/test_search.cpp
git commit -m "feat(engine): search returns a principal variation and reached depth"
```

---

### Task 2: Emit the UCI info line (score cp/mate, nodes, nps, time, pv)

**Files:**
- Modify: `engine/src/uci.cpp` (`handle_go`; add a `format_score` helper)
- Test: `engine/tests/test_uci.cpp` (update the two `go` tests that assume output starts with `bestmove`; add an info-content test)

**Interfaces:**
- Consumes: `SearchResult.pv`, `.score`, `.depth` (Task 1); `nodes_searched()`; `to_uci`.
- Produces: `handle_go` returns `"info depth D score cp X [or: score mate Y] nodes N nps M time T pv <uci moves>\nbestmove <uci>"`. On no legal move, returns `"bestmove 0000"` unchanged.

- [ ] **Step 1: Write the failing test**

```cpp
TEST_CASE("go emits an info line with score and pv, then bestmove") {
    UciState s;
    handle_command(s, "position startpos");
    std::string r = handle_command(s, "go depth 3");
    CHECK(r.find("info ")   != std::string::npos);
    CHECK(r.find("score cp")!= std::string::npos);
    CHECK(r.find(" pv ")    != std::string::npos);
    CHECK(r.find("bestmove ") != std::string::npos);
}

TEST_CASE("go reports a mate score as score mate") {
    UciState s;
    // White: Rh1, Rg7 seals rank 7; Rh1-h8 is mate in one.
    handle_command(s, "position fen k7/6R1/8/8/8/8/8/K6R w - - 0 1");
    std::string r = handle_command(s, "go depth 2");
    CHECK(r.find("score mate") != std::string::npos);
}
```

- [ ] **Step 2: Run, verify failure**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure -R "info line|mate score"`
Expected: FAIL (current `handle_go` returns only `bestmove`, no `info`).

- [ ] **Step 3: Add `format_score` and a nodes/time-aware `handle_go`**

Add near the top of the anon namespace in uci.cpp (needs `#include "search.hpp"` already present, plus `#include <chrono>`):

```cpp
// UCI score field for a side-to-move centipawn score. Our search encodes a mate
// as +-(MATE - plies_to_mate); convert that back to "mate N" in moves.
std::string format_score(int score) {
    const int MATE = 30000, THRESH = MATE - 1000;
    if (score > THRESH)  return "mate " + std::to_string((MATE - score + 1) / 2);
    if (score < -THRESH) return "mate " + std::to_string(-((MATE + score + 1) / 2));
    return "cp " + std::to_string(score);
}
```

Rewrite `handle_go`:

```cpp
std::string handle_go(UciState& state, const std::vector<std::string>& tok) {
    using Clock = std::chrono::steady_clock;
    SearchLimits lim = compute_limits(state.board, tok);
    auto t0 = Clock::now();
    SearchResult r = search_timed(state.board, lim);
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t0).count();
    if (r.best.from == NO_SQUARE) return "bestmove 0000";

    long nodes = nodes_searched();
    long long nps = ms > 0 ? (long long)nodes * 1000 / ms : 0;
    std::string pv;
    for (const Move& m : r.pv) { if (!pv.empty()) pv += " "; pv += to_uci(m); }

    std::string info = "info depth " + std::to_string(r.depth)
        + " score " + format_score(r.score)
        + " nodes " + std::to_string(nodes)
        + " nps " + std::to_string(nps)
        + " time " + std::to_string(ms)
        + " pv " + pv;
    return info + "\nbestmove " + to_uci(r.best);
}
```

- [ ] **Step 4: Update the two existing `go` tests that assume a `bestmove`-first line**

In test_uci.cpp, the `go depth returns a legal bestmove` and `go movetime` tests use `r.rfind("bestmove ", 0) == 0` and `r.substr(9)`. Change them to locate the bestmove line rather than assume position 0:

```cpp
TEST_CASE("go depth returns a legal bestmove") {
    UciState s;
    handle_command(s, "position startpos");
    std::string r = handle_command(s, "go depth 2");
    size_t bm = r.find("bestmove ");
    REQUIRE(bm != std::string::npos);
    std::string mv = r.substr(bm + 9);
    mv = mv.substr(0, mv.find_first_of(" \n"));
    Move m = move_from_uci(s.board, mv);
    CHECK(m.from != NO_SQUARE);
}

TEST_CASE("go movetime returns a bestmove quickly") {
    UciState s;
    handle_command(s, "position startpos");
    std::string r = handle_command(s, "go movetime 50");
    CHECK(r.find("bestmove ") != std::string::npos);
}
```

- [ ] **Step 5: Build and run the full suite**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: ALL PASS, including the two new info tests and the two updated `go` tests.

- [ ] **Step 6: Smoke-test the exe over UCI (info line visible)**

Run (PowerShell, note the leading blank line for the stdin quirk):
```powershell
"`nposition startpos`ngo depth 6`nquit" | & engine/build/chess_engine.exe
```
Expected: an `info depth 6 score cp ... pv ...` line, then `bestmove ...`.

- [ ] **Step 7: Commit**

```bash
git add engine/src/uci.cpp engine/tests/test_uci.cpp
git commit -m "feat(engine): emit a UCI info line with score, nodes, and pv"
```

---

## Phase 2: WASM build + coach integration (BLOCKED on Emscripten install)

Prerequisite (run once, outside this repo):
```bash
git clone https://github.com/emscripten-core/emsdk.git ~/dev/emsdk
cd ~/dev/emsdk && ./emsdk install latest && ./emsdk activate latest
```
Then `source ~/dev/emsdk/emsdk_env.sh` (or `emsdk_env.ps1`) so `emcc`/`emcmake` are on PATH. Verify with `emcc --version`.

### Task 3: WASM entry point + CMake Emscripten target

**Files:**
- Create: `engine/src/wasm_api.cpp`
- Modify: `engine/CMakeLists.txt` (add an `if(EMSCRIPTEN)` branch)

**Interfaces:**
- Produces: an exported C function `const char* uci_command(const char* line)` that drives a single persistent `UciState` through `handle_command` and returns the response text. Build output: `chesscoach.js` (ES6 module factory) + `chesscoach.wasm`.

- [ ] **Step 1: Write `engine/src/wasm_api.cpp`**

```cpp
// WebAssembly entry: one persistent UCI session driven by handle_command.
// The browser worker calls uci_command(line) and forwards each returned line.
#include "uci.hpp"
#include <string>

static UciState g_state;

extern "C" {
const char* uci_command(const char* line) {
    static std::string out;      // kept alive until the next call
    out = handle_command(g_state, std::string(line ? line : ""));
    return out.c_str();
}
}
```

- [ ] **Step 2: Add the Emscripten branch to engine/CMakeLists.txt**

Wrap the native-only targets (`engine_tests`, `chess_engine`, `enable_testing`/`add_test`) so they are skipped under Emscripten, and add the wasm target. The `engine` library stays common:

```cmake
if(EMSCRIPTEN)
    add_executable(chesscoach src/wasm_api.cpp)
    target_link_libraries(chesscoach PRIVATE engine)
    set_target_properties(chesscoach PROPERTIES
        OUTPUT_NAME chesscoach
        LINK_FLAGS "-sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=web,worker \
-sEXPORTED_FUNCTIONS=['_uci_command','_malloc','_free'] \
-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap'] -sALLOW_MEMORY_GROWTH=1")
    # Emit straight into the web app's served assets.
    set_target_properties(chesscoach PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../web/public/engine")
else()
    # ... existing engine_tests + chess_engine targets unchanged ...
endif()
```

- [ ] **Step 3: Configure and build the WASM target**

Run (from repo root, with emsdk env sourced):
```bash
emcmake cmake -S engine -B engine/build-wasm -G Ninja
cmake --build engine/build-wasm
```
Expected: `web/public/engine/chesscoach.js` and `chesscoach.wasm` are produced. Add `engine/build-wasm/` to `.gitignore`.

- [ ] **Step 4: Commit (source + build wiring; the built .js/.wasm are committed like the vendored Stockfish)**

```bash
git add engine/src/wasm_api.cpp engine/CMakeLists.txt .gitignore web/public/engine/chesscoach.js web/public/engine/chesscoach.wasm
git commit -m "feat(engine): compile the engine to WASM via an Emscripten CMake target"
```

### Task 4: Worker glue that speaks the coach's contract

**Files:**
- Create: `web/public/engine/chesscoach-worker.js`

**Interfaces:**
- Consumes: `chesscoach.js` factory + `uci_command` export.
- Produces: a Worker that, on each posted UCI command string, calls `uci_command` and posts back each non-empty output line (so `info ...` and `bestmove ...` arrive as separate messages, exactly as engine.ts expects).

- [ ] **Step 1: Write the worker (ES module worker)**

```js
// Worker wrapper so the coach can drive our WASM engine with the same UCI
// message contract it uses for Stockfish: post a command string, receive
// one text line per message.
import createModule from './chesscoach.js';

const mod = await createModule();
const uci = mod.cwrap('uci_command', 'string', ['string']);

self.onmessage = (e) => {
    const res = uci(String(e.data));
    if (!res) return;
    for (const line of res.split('\n'))
        if (line) self.postMessage(line);
};
```

- [ ] **Step 2: Manual check deferred to Task 5 (worker only runs wired into the app).**

### Task 5: "Play our engine" feature in the coach

Decision (2026-09-05): our engine is too slow/weak to replace Stockfish for depth-12
analysis (depth 6 startpos ~24s native, WASM ~2x slower). So Stockfish stays the analysis
engine, and our WASM engine powers a new "play a game against your engine" feature at a
fixed short think time. This is the honest use of the UCI seam and needs no depth-12 speed.

**Files:**
- Create: `web/src/play.ts` (a thin driver: our-engine Worker + a movable chessground board + chess.js arbiter)
- Modify: `web/src/main.ts` (a "Play the engine" tab/section entry point), `web/src/style.css` as needed
- Reuse: the existing chessground setup patterns from the drill board; the Worker contract from engine.ts (but a separate Worker instance for our engine, ES module: `new Worker('/engine/chesscoach-worker.js', { type: 'module' })`).

**Interfaces:**
- Our engine is driven with `position startpos moves <...>` then `go movetime 500`, reading `bestmove`. (Same message contract the referee match.mjs already proves works against chess_engine.exe.)

Notes for the implementer (finalize against main.ts's current UI when unblocked):
- Keep it separate from the analysis flow; do not touch engine.ts's Stockfish path.
- Human picks a color; on the engine's turn, post the move list, wait for `bestmove`, apply it via chess.js, update the board. Detect game end via chess.js.
- A fixed `movetime` (400 to 800 ms) keeps our engine responsive; no depth needed.

- [ ] **Step 1: Build a minimal play loop** (human vs engine, one color), verified in the browser: legal game start to finish against our WASM engine.

- [ ] **Step 2: Type-check and run web tests**

Run (from web/): `npx tsc --noEmit && npm run test`. Expected: green (new code is additive; analysis path untouched).

- [ ] **Step 3: Commit**

```bash
git add web/src/play.ts web/src/main.ts web/src/style.css web/public/engine/chesscoach-worker.js
git commit -m "feat(coach): play a game against our WASM engine"
```

---

## Self-Review

- **Spec coverage:** info line with score (Task 2) is the coach's hard requirement; PV + depth (Task 1) feed it; WASM entry (Task 3), worker glue (Task 4), "play the engine" feature (Task 5, reframed 2026-09-05 after the perf finding). Fuller UCI = rich info fields (score cp/mate, nodes, nps, time, pv); multipv + per-depth streaming deferred as unused (noted in Global Constraints). All covered.
- **Status (2026-09-05):** Phase 1 (Tasks 1-2) COMPLETE and committed. Phase 2 (Tasks 3-5) BLOCKED on the Emscripten install.
- **Placeholder scan:** none; Phase 2 commands are concrete and validated once emsdk is installed.
- **Type consistency:** `SearchResult.pv`/`.depth` defined in Task 1 and consumed in Task 2; `uci_command` defined in Task 3 and consumed in Task 4; `ENGINE_URL`/worker options in Task 5 match engine.ts.
- **Blocker:** Phase 2 requires Emscripten (`emcc` absent as of 2026-09-05). Phase 1 is fully unblocked.
```
