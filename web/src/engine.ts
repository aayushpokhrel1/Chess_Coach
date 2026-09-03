import { parseInfo, parseBestMove, type Info, type Score } from './uciParse';

const ENGINE_URL = '/stockfish/stockfish-18-lite-single.js';

export interface Analysis {
  score: Score;
  bestMove: string;
  pv: string[];
}

export class Engine {
  private worker: Worker;
  private ready: Promise<void>;

  constructor() {
    this.worker = new Worker(ENGINE_URL);
    this.ready = new Promise<void>((resolve) => {
      const onMsg = (e: MessageEvent) => {
        if (String(e.data).includes('uciok')) {
          this.worker.removeEventListener('message', onMsg);
          resolve();
        }
      };
      this.worker.addEventListener('message', onMsg);
      this.worker.postMessage('uci');
    });
  }

  // Analyze one position to `depth`, resolving with the final score, best move, and pv.
  async analyze(fen: string, depth: number): Promise<Analysis> {
    await this.ready;
    return new Promise<Analysis>((resolve) => {
      let last: Info | null = null;
      const onMsg = (e: MessageEvent) => {
        const line = String(e.data);
        const info = parseInfo(line);
        if (info) last = info;
        const best = parseBestMove(line);
        if (best) {
          this.worker.removeEventListener('message', onMsg);
          resolve({ score: last?.score ?? {}, bestMove: best, pv: last?.pv ?? [] });
        }
      };
      this.worker.addEventListener('message', onMsg);
      this.worker.postMessage(`position fen ${fen}`);
      this.worker.postMessage(`go depth ${depth}`);
    });
  }

  quit() {
    this.worker.postMessage('quit');
    this.worker.terminate();
  }
}
