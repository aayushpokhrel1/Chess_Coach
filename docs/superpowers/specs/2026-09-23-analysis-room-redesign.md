# Analysis Room: coach web redesign

Date: 2026-09-23
Status: approved, ready for planning
Scope: `web/` only. The C++ engine is untouched.

## Why

The coach UI is plain, and repainting it will not fix that. The causes are structural:

1. The first viewport is a form. Three native dropdowns and a button on an empty field.
   Nothing chess-related is on screen until the visitor clicks New game. The analyze tab
   opens on a bare textarea.
2. The board, the most beautiful object in the product, is 480px parked mid-page with the
   same visual weight as a select menu.
3. There is no type scale. Everything sits between 0.8rem and 1.75rem, so nothing has
   emphasis.
4. There is no material: one flat tone, one faint shadow, no light, no texture, no depth.
5. There is no motion.
6. The controls are restyled native HTML, so the page reads as a settings panel.

This spec replaces the visual world rather than refining it. `DESIGN.md` and
`.impeccable/design.json` are replaced as part of the work.

## Direction

**"The Analysis Room."** The site is a dark room with a lit board in it. The board is the
subject, the chrome recedes, and the engine's work is made visible as instrumentation.
Folded in from a second direction: the coaching explanation is promoted from a sidebar box
to an editorial annotation set inside the game's notation.

Product truth is unchanged. See `PRODUCT.md`: the primary user is a public sub-1400
beginner who has just finished a game, the two modes stay Play and Analyze, explanations
stay beginner-language and derived from the engine's line, and neither the backend
question nor the analyst's identity is settled by this work.

### Confirmed decisions

- Dark only. No light mode, no `prefers-color-scheme` variant.
- Board squares recolored to a desaturated slate-green pair; cburnett pieces unchanged.
- The landing viewport autoplays a famous miniature on a loop.
- Full markup restructure, custom controls, editorial explanation column, authored motion.
- One signature color, Lamp Gold, used as light rather than as decoration.

## Design system

### Colors

All contrast figures are against the room tone `#121816` and were verified during design.

Room:

| Token | Value | Role |
|---|---|---|
| `void` | `#0b0f0e` | Page base, behind the room. Also the text color on Lamp fills. |
| `room` | `#121816` | The default surface. |
| `raised` | `#1a2220` | Panels, the score column, popovers. |
| `edge-light` | `rgba(255,255,255,0.06)` | 1px top highlight on a raised panel. |
| `hairline` | `rgba(255,255,255,0.09)` | Dividers. |

Text:

| Token | Value | Role | Contrast |
|---|---|---|---|
| `text-bright` | `#f2ece1` | Display, headings, active state. | 14.9:1 |
| `text` | `#cfc8bc` | Body and notation. | 10.2:1 |
| `text-muted` | `#8e877c` | Secondary, captions, status. | 5.0:1 |
| `text-faint` | `#6e675e` | Disabled and placeholder only. | 3.2:1 |

Signature:

| Token | Value | Role |
|---|---|---|
| `lamp` | `#f0d9a8` | Active tab, focus ring, primary button fill, eval graph line, links, hero rim light. 13.0:1. |
| `lamp-dim` | `#d4bc89` | Hover and pressed states of Lamp elements. |
| `lamp-glow` | `rgba(240,217,168,0.14)` | Radial light gradients and glow. |
| `lamp-ring` | `rgba(240,217,168,0.35)` | Focus ring. |

Board (not UI colors):

| Token | Value |
|---|---|
| `board-dark` | `#4a5a52` |
| `board-light` | `#b3bfb6` |

Verdict, the only saturated hue in the interface:

| Token | Value | Role | Contrast |
|---|---|---|---|
| `verdict-sound` | `#4ea87a` | best and good | 5.7:1 |
| `verdict-inaccuracy` | `#d9a441` | inaccuracy | 8.0:1 |
| `verdict-mistake` | `#e07a3c` | mistake | 5.6:1 |
| `verdict-blunder` | `#e0524a` | blunder | 4.7:1 |

**The Lamp Never Judges Rule.** Lamp Gold marks structure and state: where you are, what is
focused, what the primary action is, and where the eval line runs. The verdict scale marks
how good a move was. Lamp never appears on a move, a badge, or a graph dot. Verdict never
appears on a control, a tab, or a focus ring. Lamp is always pale; verdict is always
saturated. That saturation gap is what keeps Lamp and `verdict-inaccuracy` apart, since they
share a hue family.

**The Rationed Light Rule.** Lamp covers under 8% of any screen. In a dark room a pale warm
tone is extremely loud; two Lamp elements competing in one region means one of them is
decoration and should be `text-bright` instead.

### Typography

Three families, each with a job.

- **Fraunces Variable** (`@fontsource-variable/fraunces`): display and reading. High optical
  contrast at display sizes, a warm book serif at reading sizes.
- **Inter Variable** (already installed): the interface. Buttons, labels, controls, status.
- **ui-monospace**: notation, clocks, and the engine readout.

| Role | Family | Size | Weight | Tracking | Leading |
|---|---|---|---|---|---|
| display | Fraunces | `clamp(3rem, 7vw, 5rem)` | 600 | -0.03em | 0.98 |
| headline | Fraunces | `clamp(1.6rem, 3vw, 2.25rem)` | 600 | -0.02em | 1.15 |
| annotation | Fraunces | 1.15rem | 400 | normal | 1.6 |
| title | Inter | 1rem | 600 | normal | 1.4 |
| ui | Inter | 0.9375rem | 400 | normal | 1.5 |
| label | Inter | 0.75rem | 600 | 0.06em | 1.2 |
| notation | mono | 0.9rem | 500 | normal | 1.4 |
| readout | mono | 0.8rem | 500 | 0.02em | 1.3 |

Label is uppercase. Notation, readout, clocks, and every counter use
`font-variant-numeric: tabular-nums`. The annotation column is capped at 62 to 68ch.

### Shape and spacing

Radii: 4px (inline marks), 8px (controls), 14px (panels), 999px (pills). The board has no
radius.

Spacing scale: 4 / 8 / 12 / 16 / 24 / 32 / 48 / 72.

### Elevation

A drop-shadowed panel on a dark field reads as a grey hole, so panels do not use shadows. A
panel is the `raised` tone plus a 1px `edge-light` top highlight, which reads as light
catching an edge.

Real shadows are reserved for objects that are physically above the room:

- Board cast shadow: `0 24px 60px rgba(0,0,0,0.55)`.
- Popover and dropdown listbox: `0 12px 32px rgba(0,0,0,0.5)`.
- Focus ring: `0 0 0 3px var(--lamp-ring)`.

### Atmosphere

Three layers make the room a room rather than a dark rectangle:

1. A radial `lamp-glow` gradient positioned above the board.
2. A vignette: a fixed, pointer-events-none radial overlay darkening toward the edges.
3. Film grain: an inline SVG `feTurbulence` overlay at 2 to 3% opacity, fixed, above the
   background and below the content. It also removes banding from the two gradients.

### Motion

| Token | Value | Use |
|---|---|---|
| `ease-out-expo` | `cubic-bezier(0.16, 1, 0.3, 1)` | Everything authored. |
| `t-state` | 150ms | Hover, focus, color state. |
| `t-piece` | 240ms | Piece movement. |
| `t-draw` | 700ms | Eval graph draw-on. |
| `t-hero-move` | 1100ms | Interval between hero loop moves. |

The authored moment is **analysis completing**: the eval graph draws itself left to right
via `stroke-dashoffset`, then the verdict flares pop in staggered by 40ms each. This is the
reveal of your game's shape and it happens once per analysis.

Supporting motion: the hero loop, the eval bar swinging with a slight overshoot, and state
transitions.

Under `prefers-reduced-motion: reduce`: the hero loop does not run and shows its final
position, the graph renders complete without drawing, flares appear without stagger, and all
transitions collapse to 1ms.

## Layout

Page max width 1280px. Gutters 24px, 16px under 560px.

### Hero

`min(72vh, 720px)`, minimum 560px tall. A 560px view-only board sits center-left with its
cast shadow and a Lamp rim light, autoplaying a miniature. The display headline and one
supporting line sit beside it, with two entry actions: "Play the engine" and "Analyze your
game". These are the primary navigation. Under 1024px the headline stacks above the board.

The slim tab bar is not in the hero. It appears as a sticky bar once the visitor scrolls
past the hero or activates either entry action.

### Play surface

Board 560px centered. Clocks flank it as large tabular numerals. Below the board, a mono
readout strip showing the engine's live depth, nodes, and nps while it thinks. Controls sit
above the board as a designed toolbar.

### Analyze surface

Two columns, 48px gap, stacking under 1024px:

- **Left, 560px fixed:** eval bar and board, then the move navigation.
- **Right, flexible, min 420px:** the score column.

The eval graph spans the full width above both columns.

### The score column

This replaces the current 320px sidebar with its explanation well and separate move list.

The game runs as a single typographic column in notation. Under any move that was an
inaccuracy, mistake, or blunder, its explanation is set directly beneath it: indented,
in the `annotation` role, preceded by a 2px vertical hairline in that move's verdict color.
Good moves carry no annotation and no badge.

The currently selected move's annotation is expanded and its row is marked with the Lamp
left edge at 1px. Other annotations render collapsed to two lines with a fade, expanding on
selection. Clicking any move still navigates the board.

The best line for the selected move renders in the `notation` role beneath its annotation.

## Components

### Custom dropdown

Replaces every native `<select>`. A button showing the current value plus a chevron, and a
listbox popover. Requirements:

- Full keyboard operation: Enter or Space opens, Up and Down move the active option, Enter
  selects, Escape closes and returns focus to the button, Home and End jump.
- Correct ARIA: `role="combobox"` with `aria-expanded` and `aria-controls` on the button,
  `role="listbox"` and `role="option"` with `aria-selected` in the popover.
- Closes on outside click and on scroll.
- Emits a `change` event so existing call sites keep working.

### Level picker

A dedicated control for engine strength. Each level shows its measured Elo as a large
tabular numeral plus a one-line character description. Levels and their Elo come from the
existing gauntlet data already in the app; no new ratings are invented.

### Segmented control

Time control selection. A pill-shaped row of options, the selected one filled with `raised`
and `text-bright`, the indicator sliding between options.

### Engine readout

A mono strip in `readout` role and `lamp-dim` color, showing depth, nodes, and nps while the
engine searches. Hidden when idle. Values come from the UCI `info` line the engine already
emits.

### Eval bar

Vertical, 14px, full board height, 8px radius. `board-dark` territory above, `board-light`
fill below, height transitioning at 240ms with a slight overshoot. It wears the board's
colors because it is the board's instrument.

### Eval graph

Full width, 160px tall, `raised` panel. Black's territory is the panel tone, White's is a
lighter fill, the midline is a dashed hairline, and the eval line is Lamp at 1.5px with a
soft Lamp glow beneath it. Verdict flares mark inaccuracies, mistakes, and blunders. The
cursor line is `text-bright`.

### Empty states

- **Analyze, no game:** a designed panel inviting a paste, with Lichess and Chess.com import
  as first-class actions rather than a dropdown plus a text field. No logos are used, only
  the service names as text.
- **Play, no game:** the board shows the starting position with the level picker prominent,
  rather than showing nothing until New game is pressed.

## Files

New:

- `web/src/hero.ts` — the landing loop: a hardcoded miniature PGN, move scheduling, reduced
  motion handling, and teardown on interaction.
- `web/src/controls.ts` — the custom dropdown, segmented control, and level picker.

Rewritten:

- `web/index.html` — hero section, sticky tab bar, restructured play and analyze surfaces,
  the score column, the readout strip, the grain and vignette overlays.
- `web/src/style.css` — the full token layer and every component.

Touched:

- `web/src/main.ts` — score column rendering (annotations grouped under their moves),
  readout wiring, graph draw-on trigger, control instantiation.
- `web/src/board.ts` — square color override for the recolored board.
- `web/src/play.ts` — readout data from the UCI info line, level picker integration.

Replaced:

- `DESIGN.md` and `.impeccable/design.json` — this is a redesign, so the previous Club Room
  world is replaced rather than amended.

Added dependency: `@fontsource-variable/fraunces`.

## Testing

Unit (Vitest), alongside the existing 38 tests:

- `hero.ts`: move scheduling produces the expected sequence and stops on teardown; reduced
  motion yields the final position immediately.
- `main.ts` annotation grouping: given moves and analyses, only inaccuracy, mistake, and
  blunder produce an annotation, each attached to the correct move index.
- `controls.ts`: dropdown keyboard navigation (open, arrow, select, escape) against a
  jsdom instance, and that selection emits `change`.

Verification, once per phase group rather than continuously:

- `npx tsc --noEmit` clean and the full Vitest suite green.
- A browser pass at desktop and 375px checking the hero, both surfaces, the score column,
  focus rings, and the reduced-motion path.
- The Impeccable detector over the changed files.

## Phases

Each phase leaves the app working and committed.

1. **Room and type foundation.** Tokens, Fraunces, global surfaces, board recolor, grain and
   vignette. The existing layout survives, rendered in the dark room.
2. **The score column.** Annotation grouping and rendering; the sidebar well and the old move
   list are removed.
3. **The hero.** Autoplay loop, entry actions, sticky tab bar.
4. **Controls.** Dropdown, segmented control, level picker, empty states.
5. **Instruments and motion.** Readout strip, eval graph rebuild with draw-on, eval bar
   overshoot, reduced-motion path.
6. **Record and verify.** Replace `DESIGN.md` and the sidecar from the shipped result, run
   the full verification pass.

## Out of scope

- Any change to the C++ engine or to the WASM build.
- A backend, or moving analysis off the browser.
- Replacing Stockfish as the analyst.
- A light mode.
