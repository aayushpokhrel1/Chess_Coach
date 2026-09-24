export interface HeroLoopOptions {
  fens: string[];
  onPosition: (fen: string, index: number) => void;
  intervalMs?: number;
  reducedMotion?: boolean;
}
export interface HeroLoop {
  start(): void;
  stop(): void;
}

// The landing board replays a miniature so the first viewport is never empty.
// Under reduced motion it shows the finished position instead of looping.
export function createHeroLoop(options: HeroLoopOptions): HeroLoop {
  const { fens, onPosition, intervalMs = 1100, reducedMotion = false } = options;
  let timer: ReturnType<typeof setInterval> | undefined;
  let i = 0;

  return {
    start() {
      if (reducedMotion) {
        onPosition(fens[fens.length - 1], fens.length - 1);
        return;
      }
      i = 0;
      onPosition(fens[0], 0);
      timer = setInterval(() => {
        i = (i + 1) % fens.length;
        onPosition(fens[i], i);
      }, intervalMs);
    },
    stop() {
      if (timer !== undefined) clearInterval(timer);
      timer = undefined;
    },
  };
}