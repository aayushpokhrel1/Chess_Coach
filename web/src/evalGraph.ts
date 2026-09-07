// Pure geometry for the chess.com-style evaluation graph: turn the per-move
// White-perspective evals into SVG coordinates, plus a list of marker dots at the
// moves that went wrong. No DOM here so it can be unit-tested; main.ts renders it.
import type { Quality } from './classify';

export interface GraphPoint {
  whiteEvalCp: number; // eval after the move, White's perspective (+ = White better)
  quality: Quality;
}

export interface GraphNode {
  x: number;
  y: number;
  index: number; // index into the moves array, for click-to-jump and highlighting
}

export interface GraphMarker extends GraphNode {
  quality: Quality;
}

export interface Graph {
  width: number;
  height: number;
  midY: number;
  nodes: GraphNode[]; // every move, in order (used for the line, hit-testing, highlight)
  linePoints: string; // "x,y x,y ..." for an SVG <polyline>
  whiteArea: string; // filled path: White's territory is the area under the line
  markers: GraphMarker[]; // only inaccuracy / mistake / blunder get a dot
}

// Squash centipawns into [-1, 1] so a forced mate or a +2000 rout does not flatten
// every normal swing against the top of the chart. tanh is smooth and near-linear
// around 0 (where the interesting swings live), saturating gently past a rook up.
export function squash(cp: number): number {
  return Math.tanh(cp / 400);
}

const WORST: Quality[] = ['inaccuracy', 'mistake', 'blunder'];

export function buildGraph(points: GraphPoint[], width = 640, height = 120): Graph {
  const midY = height / 2;
  const n = points.length;
  const pad = 3; // keep the line off the very edges
  const xAt = (i: number) => (n <= 1 ? width / 2 : (i / (n - 1)) * width);
  const yAt = (cp: number) => midY - squash(cp) * (midY - pad); // White up

  const nodes: GraphNode[] = points.map((p, i) => ({
    x: xAt(i),
    y: yAt(p.whiteEvalCp),
    index: i,
  }));

  const linePoints = nodes.map((p) => `${p.x.toFixed(1)},${p.y.toFixed(1)}`).join(' ');

  // White's territory = the region below the eval line (it grows as White leads).
  const whiteArea =
    n === 0
      ? ''
      : `M 0,${height} ` +
        nodes.map((p) => `L ${p.x.toFixed(1)},${p.y.toFixed(1)}`).join(' ') +
        ` L ${width},${height} Z`;

  const markers: GraphMarker[] = nodes
    .map((node, i) => ({ ...node, quality: points[i].quality }))
    .filter((m) => WORST.includes(m.quality));

  return { width, height, midY, nodes, linePoints, whiteArea, markers };
}
