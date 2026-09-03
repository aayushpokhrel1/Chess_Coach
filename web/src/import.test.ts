import { describe, it, expect } from 'vitest';
import { archivePgns } from './import';

describe('archivePgns', () => {
  it('pulls the pgn out of each game', () => {
    expect(archivePgns({ games: [{ pgn: 'A' }, { pgn: 'B' }] })).toEqual(['A', 'B']);
  });
  it('handles a missing or empty games list', () => {
    expect(archivePgns({})).toEqual([]);
    expect(archivePgns({ games: [{}, { pgn: '' }] })).toEqual([]);
  });
});
