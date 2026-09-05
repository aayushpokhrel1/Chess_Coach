// Play a full game against OUR WASM engine (separate from the Stockfish
// analysis path in engine.ts). A chessground board for the human, chess.js as
// the arbiter, and our engine driven over the same UCI worker contract as
// Stockfish: `position startpos moves ...` then `go movetime`, read `bestmove`.
import { Chessground } from 'chessground';
import type { Api } from 'chessground/api';
import { Chess } from 'chess.js';
import { legalDests } from './drill';

const $ = (id: string) => document.getElementById(id)!;
const MOVETIME = 500; // ms the engine thinks per move; enough to play, still snappy

// One request/response over the engine worker: send the move list, ask it to
// move, resolve with the bestmove uci string (e.g. "e2e4", "e7e8q").
function askEngine(worker: Worker, moves: string[]): Promise<string> {
  return new Promise((resolve) => {
    const onMsg = (e: MessageEvent) => {
      const m = String(e.data).match(/^bestmove (\S+)/);
      if (m) {
        worker.removeEventListener('message', onMsg);
        resolve(m[1]);
      }
    };
    worker.addEventListener('message', onMsg);
    worker.postMessage(`position startpos moves ${moves.join(' ')}`);
    worker.postMessage(`go movetime ${MOVETIME}`);
  });
}

// Handshake: create the worker and resolve once it answers `uci` with `uciok`.
function bootEngine(): Promise<Worker> {
  const worker = new Worker(new URL('/engine/chesscoach-worker.js', import.meta.url), {
    type: 'module',
  });
  return new Promise((resolve) => {
    const onMsg = (e: MessageEvent) => {
      if (String(e.data).includes('uciok')) {
        worker.removeEventListener('message', onMsg);
        resolve(worker);
      }
    };
    worker.addEventListener('message', onMsg);
    worker.postMessage('uci');
  });
}

export function initPlay() {
  let board: Api | null = null;
  let worker: Worker | null = null;
  let game = new Chess();
  let human: 'white' | 'black' = 'white';
  const moves: string[] = []; // uci moves so far, for `position startpos moves ...`

  const setStatus = (t: string) => ($('playStatus').textContent = t);

  function gameOverText(): string | null {
    if (!game.isGameOver()) return null;
    if (game.isCheckmate()) return game.turn() === 'w' ? 'Checkmate. Black wins.' : 'Checkmate. White wins.';
    if (game.isDraw()) return 'Draw.';
    return 'Game over.';
  }

  function render() {
    const turn = game.turn() === 'w' ? 'white' : 'black';
    const humansTurn = turn === human && !game.isGameOver();
    board!.set({
      fen: game.fen().split(' ')[0],
      turnColor: turn,
      movable: {
        free: false,
        color: humansTurn ? human : undefined,
        dests: humansTurn ? legalDests(game.fen()) : new Map(),
        events: { after: onHumanMove },
      },
    });
    setStatus(gameOverText() ?? (humansTurn ? 'Your move.' : 'Engine thinking...'));
  }

  async function onHumanMove(orig: string, dest: string) {
    // ponytail: auto-queen. Under-promotion is rare in a casual game; add a
    // picker (see main.ts pickPromotion) only if it matters.
    const piece = game.get(orig as never);
    const promo = piece?.type === 'p' && (dest[1] === '8' || dest[1] === '1') ? 'q' : undefined;
    try {
      game.move({ from: orig, to: dest, promotion: promo });
    } catch {
      render(); // illegal drop somehow: snap back
      return;
    }
    moves.push(orig + dest + (promo ?? ''));
    render();
    if (!game.isGameOver()) await engineMove();
  }

  async function engineMove() {
    const uci = await askEngine(worker!, moves);
    if (uci === '0000') return; // no move (shouldn't happen: game-over is checked first)
    game.move({ from: uci.slice(0, 2), to: uci.slice(2, 4), promotion: uci.slice(4) || undefined });
    moves.push(uci);
    render();
  }

  async function newGame() {
    human = ($('playColor') as HTMLSelectElement).value === 'black' ? 'black' : 'white';
    game = new Chess();
    moves.length = 0;
    setStatus('Loading engine...');
    if (!worker) worker = await bootEngine();
    if (!board) board = Chessground($('playBoard'), { coordinates: true });
    board.set({ orientation: human });
    render();
    if (human === 'black') await engineMove(); // engine has the first move
  }

  $('newGame').addEventListener('click', () => void newGame());
}
