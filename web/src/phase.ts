export type Phase = 'opening' | 'middlegame' | 'endgame';

// Count queens, rooks, bishops, and knights (both colors) in a FEN.
function majorMinorCount(fen: string): number {
  const placement = fen.split(' ')[0];
  return (placement.match(/[qrbnQRBN]/g) ?? []).length;
}

// Opening for the first ~10 full moves; endgame once few heavy pieces remain;
// otherwise the middlegame.
export function phaseOf(ply: number, fen: string): Phase {
  if (ply < 20) return 'opening';
  if (majorMinorCount(fen) <= 6) return 'endgame';
  return 'middlegame';
}
