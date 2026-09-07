import { defineConfig } from 'vite';

// Deployed to GitHub Pages at https://<user>.github.io/Chess_Coach/, so every
// asset resolves under that subpath. `base` makes Vite emit subpath-relative URLs
// and sets import.meta.env.BASE_URL; anything referencing a public asset by URL
// must prefix BASE_URL rather than a bare leading slash (see engine.ts / play.ts).
// In dev, base is still "/" so nothing changes locally.
export default defineConfig({
  base: '/Chess_Coach/',
});
