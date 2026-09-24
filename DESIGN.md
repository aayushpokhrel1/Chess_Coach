---
name: Chess Coach
description: A dark room with a lit board in it, where the only color is the engine's verdict on your moves.
colors:
  void: "#0b0f0e"
  room: "#121816"
  raised: "#1a2220"
  text-bright: "#f2ece1"
  text: "#cfc8bc"
  text-muted: "#8e877c"
  text-faint: "#6e675e"
  lamp: "#f0d9a8"
  lamp-dim: "#d4bc89"
  board-light: "#b3bfb6"
  board-dark: "#4a5a52"
  verdict-sound: "#4ea87a"
  verdict-inaccuracy: "#d9a441"
  verdict-mistake: "#e07a3c"
  verdict-blunder: "#e0524a"
typography:
  display:
    fontFamily: "'Fraunces Variable', Fraunces, Georgia, serif"
    fontSize: "clamp(3rem, 7vw, 5rem)"
    fontWeight: 600
    lineHeight: 0.98
    letterSpacing: "-0.03em"
  headline:
    fontFamily: "'Fraunces Variable', Fraunces, Georgia, serif"
    fontSize: "clamp(1.6rem, 3vw, 2.25rem)"
    fontWeight: 600
    lineHeight: 1.15
    letterSpacing: "-0.02em"
  annotation:
    fontFamily: "'Fraunces Variable', Fraunces, Georgia, serif"
    fontSize: "1.15rem"
    fontWeight: 400
    lineHeight: 1.6
    letterSpacing: "normal"
  title:
    fontFamily: "'Inter Variable', Inter, system-ui, sans-serif"
    fontSize: "1rem"
    fontWeight: 600
    lineHeight: 1.4
    letterSpacing: "normal"
  body:
    fontFamily: "'Inter Variable', Inter, system-ui, sans-serif"
    fontSize: "0.9375rem"
    fontWeight: 400
    lineHeight: 1.5
    letterSpacing: "normal"
  label:
    fontFamily: "'Inter Variable', Inter, system-ui, sans-serif"
    fontSize: "0.75rem"
    fontWeight: 600
    lineHeight: 1.2
    letterSpacing: "0.06em"
  notation:
    fontFamily: "ui-monospace, SFMono-Regular, Menlo, monospace"
    fontSize: "0.9rem"
    fontWeight: 500
    lineHeight: 1.4
    letterSpacing: "normal"
  readout:
    fontFamily: "ui-monospace, SFMono-Regular, Menlo, monospace"
    fontSize: "0.8rem"
    fontWeight: 500
    lineHeight: 1.3
    letterSpacing: "0.02em"
rounded:
  sm: "4px"
  md: "8px"
  lg: "14px"
  pill: "999px"
spacing:
  xs: "4px"
  sm: "8px"
  md: "16px"
  lg: "24px"
  xl: "32px"
  xxl: "48px"
components:
  button-lamp:
    backgroundColor: "{colors.lamp}"
    textColor: "{colors.void}"
    typography: "{typography.title}"
    rounded: "{rounded.md}"
    padding: "12px 24px"
  button-lamp-hover:
    backgroundColor: "{colors.lamp-dim}"
    textColor: "{colors.void}"
  button-ghost:
    backgroundColor: "transparent"
    textColor: "{colors.text-bright}"
    typography: "{typography.title}"
    rounded: "{rounded.md}"
    padding: "12px 24px"
  button-ghost-hover:
    backgroundColor: "{colors.raised}"
    textColor: "{colors.text-bright}"
  button-secondary:
    backgroundColor: "{colors.raised}"
    textColor: "{colors.text}"
    typography: "{typography.title}"
    rounded: "{rounded.md}"
    padding: "8px 16px"
  panel:
    backgroundColor: "{colors.raised}"
    textColor: "{colors.text}"
    rounded: "{rounded.lg}"
    padding: "24px"
  dropdown-list:
    backgroundColor: "{colors.raised}"
    textColor: "{colors.text}"
    rounded: "{rounded.md}"
    padding: "4px"
  segment-selected:
    backgroundColor: "{colors.raised}"
    textColor: "{colors.text-bright}"
    typography: "{typography.title}"
    rounded: "{rounded.pill}"
    padding: "8px 16px"
  tab-active:
    backgroundColor: "{colors.raised}"
    textColor: "{colors.text-bright}"
    typography: "{typography.title}"
    rounded: "8px 8px 0 0"
    padding: "8px 16px"
  verdict-label:
    backgroundColor: "transparent"
    textColor: "{colors.verdict-blunder}"
    typography: "{typography.label}"
    rounded: "{rounded.sm}"
    padding: "1px 4px"
---

# Design System: Chess Coach

## Overview

**Creative North Star: "The Analysis Room"**

Chess Coach is a dark room with a lit board in it. The board is the subject and
everything else recedes: a warm near-black field (#121816), a lamp gradient above the
board, a vignette at the edges, and a film grain over all of it that kills gradient
banding and gives the room texture. The visitor arrives having just lost a game. A bright
interface would make that feel like an inspection; a dark one feels like sitting down at a
table afterwards.

The engine is made visible rather than hidden. While it searches it reports its depth,
node count, and nodes per second as a mono readout under the board. The eval bar is the
board's instrument and wears the board's colors. When an analysis completes, the eval graph
draws itself left to right and the verdict flares pop in staggered behind it. That reveal is
the one authored motion moment in the product, and it happens once per analysis.

The coaching does not live in a panel beside the game. It lives inside the notation: the
score runs as a single typographic column, and under any move that cost something, the
explanation is set in a reading serif behind a rule in that move's verdict color. Good moves
are silent. This is what a coach's markup actually looks like, and it is why the product's
best asset stopped being a grey box in a sidebar.

**Key Characteristics:**

- A dark warm room, a lit board, and light rather than hue as the interface's voice.
- Fraunces for display and for reading, Inter for the interface, mono for notation.
- Verdict color is the only saturated hue anywhere.
- Panels are lit edges, never drop shadows. On a dark field a shadow reads as a hole.
- The engine's work is shown, not hidden.

## Colors

A warm monochrome room with one pale light and one saturated scale. Contrast figures are
against the room tone (#121816).

### Primary

- **Lamp Gold** (`#f0d9a8`): the signature, and the room's light source made into the brand.
  Primary buttons, the active tab underline, focus rings, links, the eval graph's line, and
  the rim light on the hero board. 13.0:1, and it carries `void` text at 13.8:1.
- **Dim Lamp** (`#d4bc89`): the hovered and pressed state of anything Lamp, plus the engine
  readout. Never used at rest on a surface.

### Secondary

- **Board Light** (`#b3bfb6`) and **Board Dark** (`#4a5a52`): a desaturated slate pair so
  the board belongs to the room instead of glowing out of it. cburnett pieces are unchanged.
  These appear on the board and the eval bar and nowhere else.

### Tertiary

The verdict scale, the only saturated hue in the interface.

- **Sound** (`#4ea87a`, 5.7:1): best and good moves. Rarely rendered, since good moves are silent.
- **Inaccuracy** (`#d9a441`): 8.0:1.
- **Mistake** (`#e07a3c`): 5.6:1.
- **Blunder** (`#e0524a`): 4.7:1. The reddest thing on screen, because it is the most
  important thing on screen.

### Neutral

- **Void** (`#0b0f0e`): the page base behind the room, and the text color on Lamp fills.
- **Room** (`#121816`): the default surface.
- **Raised** (`#1a2220`): panels, the dropdown listbox, the eval graph, selected segments.
- **Edge Light** (`rgba(255,255,255,0.06)`): the 1px top highlight that makes a panel read
  as lifted.
- **Hairline** (`rgba(255,255,255,0.09)`): dividers and control borders.
- **Bright** (`#f2ece1`): display, headings, active state, the graph cursor. 14.9:1.
- **Text** (`#cfc8bc`): body and notation. 10.2:1.
- **Muted** (`#8e877c`): secondary text, captions, status. 5.0:1.
- **Faint** (`#6e675e`): disabled controls and placeholders only.

### Named Rules

**The Lamp Never Judges Rule.** Lamp Gold marks structure and state: where you are, what is
focused, what the primary action is, and where the eval line runs. The verdict scale marks
how good a move was. Lamp never appears on a move, a badge, or a graph dot. Verdict never
appears on a control, a tab, or a focus ring. Lamp is always pale; verdict is always
saturated. That saturation gap is what keeps Lamp and Inaccuracy apart, since they share a
hue family.

**The Rationed Light Rule.** Lamp covers under 8% of any screen. In a dark room a pale warm
tone is extremely loud. If two Lamp elements compete for attention in one region, one of
them is decoration and should be Bright instead.

**The Board Owns Its Own Colors Rule.** Board Light and Board Dark appear on the board and
the eval bar. Painting UI chrome in them turns the room into a theme.

## Typography

**Display and Reading Font:** Fraunces Variable (with Fraunces, Georgia, serif)
**Interface Font:** Inter Variable (with Inter, system-ui, sans-serif)
**Notation Font:** ui-monospace (with SFMono-Regular, Menlo, monospace)

**Character:** Fraunces does the two jobs that carry the product's voice. At 5rem with high
optical contrast it is cinematic, and at 1.15rem it is a warm book serif, which is what the
coaching explanation needed to stop reading like a system message. Inter handles every
control and label, so the interface never competes with the writing. Monospace is not a
technical costume here: it marks text that is literally chess notation, plus the clocks and
the engine's own numbers.

### Hierarchy

- **Display** (Fraunces 600, `clamp(3rem, 7vw, 5rem)`, 0.98, -0.03em): the hero headline.
  One per page.
- **Headline** (Fraunces 600, `clamp(1.6rem, 3vw, 2.25rem)`, 1.15, -0.02em): panel titles,
  the report insight, the drill prompt.
- **Annotation** (Fraunces 400, 1.15rem, 1.6): the coaching explanation inside the score.
  Capped at 64ch.
- **Title** (Inter 600, 1rem, 1.4): buttons, tabs, table captions.
- **Body** (Inter 400, 0.9375rem, 1.5): interface prose.
- **Label** (Inter 600, 0.75rem, 0.06em, uppercase): verdict labels, field labels, status.
- **Notation** (mono 500, 0.9rem, tabular): SAN moves, best lines, clocks, eval numbers.
- **Readout** (mono 500, 0.8rem, 0.02em, tabular, Dim Lamp): the engine's live search.

### Named Rules

**The Tabular Figures Rule.** Anything that counts or changes in place (clocks, evaluations,
node counts, move indices, drill progress) uses `font-variant-numeric: tabular-nums`. A
clock that reflows every second is a defect.

**The Notation Is Not Decoration Rule.** Monospace is reserved for SAN, FEN, clocks, eval
values, and the engine readout. Never for headings, buttons, or prose.

## Layout

Page max width 1280px, centered, with 24px gutters.

The hero is a two-column grid, copy left and a 560px board right, `min(72vh, 720px)` tall.
It stacks under 1024px, where the board becomes `min(560px, 100vw - 3rem)` and keeps its 1:1
aspect ratio.

The tab bar is sticky, sitting on a hairline with the room tone behind it so content scrolls
under rather than through it.

The analyze surface is two columns with a 48px gap: a 480px board column carrying the eval
bar and move navigation, and a flexible score column with a 320px floor. It stacks under
1024px, score column below the board.

Spacing follows a 4px base: 4 / 8 / 12 / 16 / 24 / 32 / 48. Control internals use 8 and 16,
sibling controls sit 8 apart, panels 16, sections 24 to 48.

### Named Rules

**The Board Never Shrinks Below Its Squares Rule.** The board keeps a 1:1 aspect ratio and
never drops below 280px. If the viewport cannot hold the board and the score column, the
score column moves below the board. It never squeezes the board.

## Elevation & Depth

Inverted from the usual. On a dark field a drop-shadowed panel reads as a grey hole, so
panels do not use shadows at all: a panel is the Raised tone plus a 1px Edge Light top
highlight, which reads as light catching an edge from the lamp above.

Real shadows are reserved for things that are physically above the room.

### Shadow Vocabulary

- **Board cast** (`0 24px 60px rgba(0,0,0,0.55)`): under every board. It is the one object
  in the room with real weight.
- **Popover** (`0 12px 32px rgba(0,0,0,0.5)`): the dropdown listbox, and anything floating.
- **Focus ring** (`0 0 0 3px rgba(240,217,168,0.35)`): every interactive element, never
  removed without a replacement.

### Named Rules

**The Lit Edge, Not The Dropped Shadow Rule.** Panels get `border-top: 1px solid
var(--edge-light)` and no `box-shadow`. A shadow under a panel on this field is a hole, not
a lift.

**The One Elevation Step Rule.** A surface is Room or Raised. There is no third tone.
Stacking more contrast to signal more importance is how a layout stops having a hierarchy.

## Shapes

Four radii and no more: 4px for inline marks (verdict labels, score rows), 8px for controls
(buttons, inputs, the eval bar, the dropdown), 14px for panels and the eval graph, and full
pill for the segmented control. The board is square, because a chess board has corners.

Borders are hairlines at 1px, used on controls and as the top highlight on panels. A border
and a shadow never appear on the same element.

The board's checkerboard is drawn with a conic gradient bound to the board tokens rather
than the theme stylesheet's embedded SVG, so both square colors stay live-bound to `:root`.

## Components

### Buttons

- **Lamp (primary):** Lamp Gold fill, Void text, 8px radius, 12px by 24px in the hero and
  8px by 16px elsewhere. One per region: the action that advances the task.
- **Ghost:** transparent with a hairline border and Bright text. The secondary entry action.
- **Secondary:** Raised fill, Text color, hairline border. Everything else.
- **Hover:** Lamp darkens to Dim Lamp keeping Void text; secondary and ghost lift to Raised
  with Bright text. 150ms.
- **Disabled:** Faint text, no fill change, default cursor. Disabled controls stay visible.

### Panels

Raised fill, 14px radius, 24px padding, a 1px Edge Light top highlight, and no shadow. Used
for the import panel, the paste panel, the report, the drill, and the eval graph.

### Dropdown

A button with `role="combobox"` and a listbox popover on Raised with the popover shadow.
Fully keyboard operable: Enter, Space, or the arrows open it; arrows move the active option;
Home and End jump; Enter selects; Escape closes and returns focus to the button. Closes on
outside click and on scroll.

### Level picker

One card per strength level showing the measured Elo as a large tabular numeral, the level
name, and a one-line description of how it plays. Novice reads "unrated" rather than a
fabricated number, because it plays half at random and sits below the engine's floor. The
Elo figures come from the project's own rating gauntlet.

### Segmented control

A pill row for the color and time choices. The selected segment is Raised with Bright text,
and the indicator moves with `transform`, never by animating `left` or `width`.

### Score column

An ordered column of notation. Each row is a mono SAN at 4px radius with a 1px transparent
left border that turns Lamp when the row is selected. Under any row whose move was an
inaccuracy, mistake, or blunder sits an annotation: an uppercase Label in that verdict's
color, the explanation in the Annotation role, and the engine's better line in Notation,
all behind a 2px left rule in the verdict color. Unselected annotations clamp to two lines;
selecting the move expands it and reveals the better line.

### Eval bar

A 14px column beside the board at full board height, 8px radius. Board Dark territory above,
Board Light fill below, height transitioning over 240ms. It wears the board's colors because
it is the board's instrument.

### Eval graph

A 160px full-width Raised panel. White's territory is a Board Light area at 16% opacity, the
midline is a dashed hairline, and the eval line is Lamp at 1.5px with a soft Lamp glow under
it. Verdict flares mark the inaccuracies, mistakes, and blunders. The cursor is Bright.

### Engine readout

A centered mono strip in Dim Lamp under the play board, showing the engine's live depth,
node count, and nodes per second while it searches, and hidden when it is idle.

## Do's and Don'ts

### Do:

- **Do** keep Lamp Gold under 8% of any screen, on primary actions, active navigation,
  focus, links, and the eval line only.
- **Do** use the verdict scale for move quality only, in both the score column and the eval
  graph, so the two always agree.
- **Do** build depth with the Raised tone and a lit top edge.
- **Do** put `tabular-nums` on every clock, evaluation, node count, and counter.
- **Do** keep the annotation column at 64ch and set it in Fraunces. It is the product.
- **Do** give every interactive element the Lamp focus ring.
- **Do** honor `prefers-reduced-motion`: no hero loop, no graph draw-on, no stagger.

### Don't:

- **Don't** put a `box-shadow` under a panel. On this field it reads as a hole, not a lift.
- **Don't** paint UI chrome in the board's colors.
- **Don't** add a second accent, and never bring back the old blue (#4a86e8) or the previous
  world's felt green (#3f5f4a).
- **Don't** badge good moves. Only inaccuracy, mistake, and blunder are annotated.
- **Don't** use pure black for text, shadows on surfaces, or fills. The room is warm.
- **Don't** use monospace for headings, buttons, or prose.
- **Don't** animate `left`, `width`, `height`, or `margin` for motion that could use
  `transform`. The eval bar's height is the one sanctioned exception, because it is a
  14px-wide instrument and the value it shows is the height.
- **Don't** invent a rating. Strength numbers come from the gauntlet or read "unrated".
