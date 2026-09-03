// Fetch the most recent `max` games as concatenated PGN from Lichess.
export async function fetchLichess(user: string, max: number): Promise<string> {
  const url = `https://lichess.org/api/games/user/${encodeURIComponent(user)}?max=${max}`;
  const r = await fetch(url, { headers: { Accept: 'application/x-chess-pgn' } });
  if (!r.ok) throw new Error(`Lichess request failed (${r.status})`);
  return (await r.text()).trim();
}

// Extract the PGN strings from one Chess.com monthly archive JSON object.
export function archivePgns(archive: { games?: { pgn?: string }[] }): string[] {
  return (archive.games ?? []).map((g) => g.pgn ?? '').filter((p) => p.length > 0);
}

// Fetch the most recent `max` games as concatenated PGN from Chess.com.
export async function fetchChessCom(user: string, max: number): Promise<string> {
  const a = await fetch(
    `https://api.chess.com/pub/player/${encodeURIComponent(user)}/games/archives`,
  );
  if (!a.ok) throw new Error(`Chess.com request failed (${a.status})`);
  const archives: string[] = (await a.json()).archives ?? [];
  const pgns: string[] = [];
  for (let i = archives.length - 1; i >= 0 && pgns.length < max; i--) {
    const g = await fetch(archives[i]);
    if (!g.ok) continue;
    const monthly = archivePgns(await g.json()); // oldest first within a month
    for (let j = monthly.length - 1; j >= 0 && pgns.length < max; j--) {
      pgns.push(monthly[j]);
    }
  }
  return pgns.join('\n\n');
}
