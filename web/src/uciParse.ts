export interface Score {
  cp?: number;
  mate?: number;
}
export interface Info {
  depth: number;
  score: Score;
  pv: string[];
}

// Parse a UCI "info ..." line. Returns null if it is not an info line or has no score.
export function parseInfo(line: string): Info | null {
  if (!line.startsWith('info')) return null;
  const t = line.split(/\s+/);
  const num = (key: string): number | undefined => {
    const i = t.indexOf(key);
    return i >= 0 ? Number(t[i + 1]) : undefined;
  };

  const score: Score = {};
  const si = t.indexOf('score');
  if (si >= 0) {
    if (t[si + 1] === 'cp') score.cp = Number(t[si + 2]);
    else if (t[si + 1] === 'mate') score.mate = Number(t[si + 2]);
  }
  if (score.cp === undefined && score.mate === undefined) return null;

  const pi = t.indexOf('pv');
  const pv = pi >= 0 ? t.slice(pi + 1) : [];
  return { depth: num('depth') ?? 0, score, pv };
}

// Parse a UCI "bestmove <move> [ponder <move>]" line. Returns null otherwise.
export function parseBestMove(line: string): string | null {
  if (!line.startsWith('bestmove')) return null;
  return line.split(/\s+/)[1] ?? null;
}
