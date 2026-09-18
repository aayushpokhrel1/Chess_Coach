import { defineConfig } from 'vite';

// GitHub Pages serves at https://<user>.github.io/Chess_Coach/ (a subpath), but
// Cloudflare Pages serves at a domain root. Cloudflare auto-sets CF_PAGES=1 during
// its build, so pick the base per host: root there, subpath on GH Pages. `base`
// makes Vite emit base-relative URLs and sets import.meta.env.BASE_URL; anything
// referencing a public asset by URL must prefix BASE_URL rather than a bare leading
// slash (see engine.ts / play.ts). In dev, base is "/" so nothing changes locally.
export default defineConfig({
  base: process.env.CF_PAGES ? '/' : '/Chess_Coach/',
});
