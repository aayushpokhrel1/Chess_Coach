import { defineConfig } from 'vite';

// Served at the root of chess-coach.pages.dev (Cloudflare), so base is '/'. `base`
// sets import.meta.env.BASE_URL, which code prefixes onto public-asset URLs (the
// Stockfish and WASM engine workers in engine.ts / play.ts), so '/' keeps those
// resolving at the site root. If this ever moves to a subpath host, set base to
// that subpath and those asset URLs follow automatically.
export default defineConfig({
  base: '/',
});
