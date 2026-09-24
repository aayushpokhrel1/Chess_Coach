---
name: Chess Coach
description: A club room for your own games: warm paper, a wooden board, and green felt for the engine's voice.
colors:
  felt: "#3f5f4a"
  felt-deep: "#334e3c"
  felt-wash: "#e7ede9"
  page: "#faf6f0"
  surface: "#ffffff"
  surface-sunken: "#f3ece2"
  hairline: "#e3d9cc"
  ink: "#23201c"
  ink-muted: "#6b6259"
  ink-faint: "#9a9086"
  board-light: "#f0d9b5"
  board-dark: "#b58863"
  verdict-best: "#1d7a44"
  verdict-inaccuracy: "#8f6200"
  verdict-mistake: "#c2410c"
  verdict-blunder: "#b3261e"
typography:
  display:
    fontFamily: "'Inter Variable', Inter, system-ui, sans-serif"
    fontSize: "1.75rem"
    fontWeight: 700
    lineHeight: 1.15
    letterSpacing: "-0.02em"
  headline:
    fontFamily: "'Inter Variable', Inter, system-ui, sans-serif"
    fontSize: "1.15rem"
    fontWeight: 650
    lineHeight: 1.3
    letterSpacing: "-0.01em"
  title:
    fontFamily: "'Inter Variable', Inter, system-ui, sans-serif"
    fontSize: "1rem"
    fontWeight: 600
    lineHeight: 1.4
    letterSpacing: "normal"
  body:
    fontFamily: "'Inter Variable', Inter, system-ui, sans-serif"
    fontSize: "0.95rem"
    fontWeight: 400
    lineHeight: 1.55
    letterSpacing: "normal"
  label:
    fontFamily: "'Inter Variable', Inter, system-ui, sans-serif"
    fontSize: "0.8rem"
    fontWeight: 600
    lineHeight: 1.2
    letterSpacing: "0.02em"
  notation:
    fontFamily: "ui-monospace, SFMono-Regular, Menlo, monospace"
    fontSize: "0.9rem"
    fontWeight: 500
    lineHeight: 1.4
    letterSpacing: "normal"
rounded:
  sm: "4px"
  md: "8px"
  lg: "12px"
spacing:
  xs: "4px"
  sm: "8px"
  md: "16px"
  lg: "24px"
  xl: "32px"
components:
  button-primary:
    backgroundColor: "{colors.felt}"
    textColor: "{colors.page}"
    typography: "{typography.title}"
    rounded: "{rounded.md}"
    padding: "8px 16px"
  button-primary-hover:
    backgroundColor: "{colors.felt-deep}"
    textColor: "{colors.page}"
  button-secondary:
    backgroundColor: "{colors.surface}"
    textColor: "{colors.ink}"
    typography: "{typography.title}"
    rounded: "{rounded.md}"
    padding: "8px 16px"
  button-secondary-hover:
    backgroundColor: "{colors.felt-wash}"
    textColor: "{colors.felt-deep}"
  input-text:
    backgroundColor: "{colors.surface-sunken}"
    textColor: "{colors.ink}"
    typography: "{typography.body}"
    rounded: "{rounded.md}"
    padding: "8px 12px"
  card:
    backgroundColor: "{colors.surface}"
    textColor: "{colors.ink}"
    rounded: "{rounded.lg}"
    padding: "16px"
  tab:
    backgroundColor: "transparent"
    textColor: "{colors.ink-muted}"
    typography: "{typography.title}"
    rounded: "8px 8px 0 0"
    padding: "8px 16px"
  tab-active:
    backgroundColor: "{colors.surface}"
    textColor: "{colors.ink}"
  chip-verdict:
    backgroundColor: "transparent"
    textColor: "{colors.verdict-blunder}"
    typography: "{typography.label}"
    rounded: "{rounded.sm}"
    padding: "1px 4px"
---

# Design System: Chess Coach

## Overview

**Creative North Star: "The Club Room"**

Chess Coach looks like the table you sit down at after a game, not like an analytics
product. The board is the classic wooden one every club and every online lobby already
uses, so a beginner recognises it before they read a word. Around it, the room is warm
paper (#faf6f0), and the only voice that belongs to the software is the green of table
felt (#3f5f4a). Nothing shouts. The visitor has just lost a game and arrived slightly
embarrassed; a loud interface would make that worse.

The system is warm, quiet, and dense with real information. Panels are white cards that
sit on the paper with a soft shadow, because the visitor is working with several distinct
objects at once: a board, a move list, an explanation, a graph, a drill. Cards make those
objects separable at a glance without borders everywhere. Typography is a single humanist
sans (Inter) doing all the talking, with monospace reserved for the things that are
literally notation: moves, clocks, evaluations.

Color is rationed hard. The felt marks structure and engine truth. A separate, saturated
verdict scale (green through red) marks how good a move was, and it appears only on move
quality. Those two families never trade jobs. Rejected outright: a dark analytics chrome,
neon accents, the arbitrary blue that used to sit on the active tab, and any decorative
gradient or glassmorphism.

**Key Characteristics:**

- Warm paper page, white cards, wooden board, green felt accent.
- One typeface family; monospace only for real notation.
- Verdict color is earned by the engine, never used for decoration.
- Soft, warm, low-contrast shadows; no hard borders doing a card's job.
- Generous line height and muted secondary text, because beginners read every word.

## Colors

A warm, low-saturation room with two loud exceptions: the board's wood, and the verdict
scale that tells you what a move cost.

### Primary

- **Table Felt** (`#3f5f4a`): The one brand voice. Active tab, primary button, focus ring,
  the eval graph's line, and links. Deep enough to carry white text at 7:1.
- **Deep Felt** (`#334e3c`): The pressed and hovered state of anything felt-colored. Never
  used at rest.
- **Felt Wash** (`#e7ede9`): A barely-there tint for the selected move row and secondary
  button hover. It is the felt at a whisper, never a fill in its own right.

### Secondary

- **Board Light** (`#f0d9b5`) and **Board Dark** (`#b58863`): Chessground's brown theme.
  These are not UI colors. They belong to the board and to anything that is literally a
  chess surface (the eval bar tracks them). Do not paint buttons, panels, or headings in
  them.

### Tertiary

The verdict scale. It exists to say what a move cost, and it says nothing else.

- **Sound** (`#1d7a44`): best and good moves.
- **Inaccuracy** (`#8f6200`): a small, recoverable error.
- **Mistake** (`#c2410c`): a real error.
- **Blunder** (`#b3261e`): the move that decided the game. Also the eval graph's blunder
  dots.

### Neutral

- **Paper** (`#faf6f0`): the page. Warm, not white, so the white cards read as objects on it.
- **Card** (`#ffffff`): every raised panel.
- **Sunken Paper** (`#f3ece2`): recessed surfaces that are containers rather than content,
  such as the explanation well and the PGN input.
- **Hairline** (`#e3d9cc`): a warm 1px divider. Never a shadow substitute.
- **Ink** (`#23201c`): body and headings. Warm near-black, never pure #000.
- **Muted Ink** (`#6b6259`): secondary text, captions, status lines, clock labels.
- **Faint Ink** (`#9a9086`): disabled controls and placeholders only.

### Named Rules

**The Felt Never Judges Rule.** Table Felt and the verdict scale are separate vocabularies.
Felt marks where you are and what the engine is showing; verdict color marks how good a
move was. A green button is a bug. A felt-colored blunder badge is a bug. Audit test: cover
the board and ask of every colored element, "is this navigation, or is this a grade?"

**The Rationed Accent Rule.** Felt covers less than 10% of any screen. If two felt elements
compete for attention in the same region, one of them is decoration and should be neutral.

**The Board Owns The Wood Rule.** Board Light and Board Dark appear on the board and the
eval bar, and nowhere else. Extending the wood into the chrome turns the room into a theme.

## Typography

**Display / Body / Label Font:** Inter Variable (with Inter, system-ui, sans-serif)
**Notation Font:** ui-monospace (with SFMono-Regular, Menlo, monospace)

**Character:** One humanist sans does all of the speaking, so the interface never sounds
like two different products. Inter is chosen for its even color at small sizes and its
tabular figures, which matter here: clocks tick, evaluations change, and neither should
make the layout jump. The monospace is not a style choice; it marks text that is literally
chess notation.

### Hierarchy

- **Display** (700, 1.75rem, 1.15, -0.02em): the page title only. One per page.
- **Headline** (650, 1.15rem, 1.3, -0.01em): the report headline insight, drill prompt,
  card titles.
- **Title** (600, 1rem, 1.4): buttons, tabs, table captions, the play status line.
- **Body** (400, 0.95rem, 1.55): explanations and prose. Cap measure at 65-75ch; the
  explanation is the product and must not run to the edge of a wide screen.
- **Label** (600, 0.8rem, 0.02em): verdict badges, field labels, secondary status.
- **Notation** (500, 0.9rem, monospace, tabular figures): SAN moves, clocks, eval numbers,
  best lines.

### Named Rules

**The Tabular Figures Rule.** Anything that counts or changes in place (clocks, evaluation
numbers, move indices, drill progress) uses `font-variant-numeric: tabular-nums`. A clock
that reflows every second is a defect.

**The Notation Is Not Decoration Rule.** Monospace is reserved for SAN, FEN, clocks, and
eval values. Never for headings, buttons, or body prose.

## Layout

A single centered column, max 1040px, with 24px page gutters and 32px of breathing room
above the fold. Inside it, the analysis view is a two-column flex: the board column
(480px, fixed, because the board is square and must not resample) and a 320px side column
carrying the explanation, best line, and move list. Below 900px the two columns stack, the
board becomes fluid (`min(480px, 100vw - 48px)`) and the side column goes full width.

Spacing follows a 4px base: 4 / 8 / 16 / 24 / 32. Component internals use 8 and 16; gaps
between sibling controls use 8; gaps between cards use 16; sections are separated by 24.

Density is moderate, not compact. The move list is the one dense region (2px row padding)
because it is scanned, not read.

### Named Rules

**The Board Never Shrinks Below Its Squares Rule.** The board keeps a 1:1 aspect ratio and
never drops below 280px. If the viewport cannot hold the board and the side column, the
side column moves below the board; it never squeezes the board.

## Elevation & Depth

Layered. Surfaces are objects: the page is paper, and every functional region is a white
card lifted off it by a soft, warm, low-opacity shadow. Depth is what separates the board
from the move list from the report, which is why this system does not need borders around
everything. Shadows are warm (they carry the ink hue, not pure black) so they do not read
as grey holes in a cream page.

### Shadow Vocabulary

- **Resting card** (`box-shadow: 0 1px 2px rgba(35,32,28,0.04), 0 2px 8px rgba(35,32,28,0.06)`):
  every panel at rest.
- **Raised** (`box-shadow: 0 2px 4px rgba(35,32,28,0.06), 0 8px 20px rgba(35,32,28,0.10)`):
  hover on an interactive card, and anything temporarily above the page.
- **Focus ring** (`box-shadow: 0 0 0 3px rgba(63,95,74,0.28)`): keyboard focus, on every
  interactive element, never removed without a replacement.

### Named Rules

**The Warm Shadow Rule.** Shadows use `rgba(35,32,28,...)`, never `rgba(0,0,0,...)`. Pure
black shadows on cream read as dirt.

**The One Elevation Step Rule.** A card is either resting or raised. There is no third
level; stacking more shadow to signal more importance is how a layout stops having a
hierarchy.

## Shapes

Soft, consistent, unfussy. Three radii and no more: 4px for small inline marks (verdict
badges, move rows), 8px for controls (buttons, inputs, selects, the eval bar), 12px for
cards and the eval graph. The board itself is square with no radius, because a chess board
has corners.

Borders are used sparingly and only where a shadow would be wrong: 1px Hairline on inputs
and sunken wells, to say "type here". Cards use shadow, not border. The two are never
combined on the same element.

## Components

### Buttons

- **Shape:** Gently rounded (8px radius), 8px by 16px padding, Title type.
- **Primary:** Table Felt background (#3f5f4a), Paper text (#faf6f0). One per region: the
  action that advances the task (Analyze, New game, Load).
- **Secondary:** White card background, Ink text, 1px Hairline border. Everything else.
- **Hover / Focus:** Primary darkens to Deep Felt (#334e3c); secondary fills to Felt Wash
  (#e7ede9) and its text goes Deep Felt. Both transition over 150ms. Focus-visible adds the
  focus ring at all times.
- **Disabled:** Faint Ink text (#9a9086), Hairline border, no background change, default
  cursor. Disabled buttons stay in place; they are never hidden.

### Cards / Containers

- **Corner Style:** 12px radius.
- **Background:** Card white (#ffffff) on Paper (#faf6f0).
- **Shadow Strategy:** Resting card by default; Raised only when the whole card is
  interactive.
- **Border:** None. The shadow is the edge.
- **Internal Padding:** 16px, 24px for the report card.

### Inputs / Fields

- **Style:** Sunken Paper background (#f3ece2), 1px Hairline border, 8px radius, 8px by
  12px padding, Body type. The PGN textarea uses Notation type because its content is
  notation.
- **Focus:** Border goes Table Felt, plus the focus ring. No glow, no color fill.
- **Placeholder:** Faint Ink.

### Navigation

- **Style:** A two-tab bar sitting on a Hairline rule. Tabs are Title type in Muted Ink,
  8px 8px 0 0 radius, transparent at rest.
- **Active:** White card background that visually joins the content below, Ink text, and a
  3px Table Felt underline sitting on the rule.
- **Hover (inactive):** Text darkens to Ink, background goes Felt Wash.
- **Mobile:** Tabs stay side by side and stretch to equal width; they never collapse into a
  menu, because there are only two and both must stay one tap away.

### Move List

The dense region. An ordered list, Notation type for the SAN, 4px radius per row, 2px
vertical padding. Hover fills Felt Wash; the current move fills Felt Wash and carries a 1px
Table Felt left edge. The verdict badge is Label type in its verdict color, sitting after
the move. Only mistakes are badged; a "good" badge on every second move is noise.

### Eval Bar

A 14px column beside the board, full board height, 8px radius, clipped. Board Dark
territory above, Board Light fill below, height transitioning over 200ms. It is the board's
instrument, which is why it wears the board's colors and not the UI's.

### Eval Graph

A 120px full-width card (12px radius) showing the game's swing. Black's territory is a warm
mid-tone (#b9ae9e), White's is a Paper area path, the midline is dashed Muted Ink at 35%,
and the eval line is Table Felt at 1.5px. Black's field is deliberately a mid-tone rather
than near-black: the felt line and the felt cursor cross both fields and must clear 3:1 on
each. Blunder, mistake, and inaccuracy dots take their verdict colors with a white stroke.

### Verdict Badge

Label type, verdict color, transparent background, 4px radius, sitting inline after a move
or on a graph dot. It is text, not a pill: a filled badge would put four saturated blocks
in a scanned list and destroy the move list's calm.

## Do's and Don'ts

### Do:

- **Do** keep Table Felt (#3f5f4a) under 10% of any screen, on navigation state, primary
  actions, focus, and the eval line only.
- **Do** use the verdict scale (#1d7a44 / #8f6200 / #c2410c / #b3261e) exclusively for move
  quality, in both the move list and the eval graph, so the two always agree.
- **Do** use `tabular-nums` on every clock, evaluation, and counter.
- **Do** cap explanation prose at 65-75ch, since the explanation is the product.
- **Do** give every interactive element a visible `:focus-visible` ring
  (`0 0 0 3px rgba(63,95,74,0.28)`).
- **Do** keep the board at 1:1 and let the side column move below it on narrow screens.

### Don't:

- **Don't** paint UI chrome in the board's wood (#f0d9b5 / #b58863). The board owns those.
- **Don't** reintroduce the old blue (#4a86e8) or add any second accent.
- **Don't** combine a border and a shadow on the same element.
- **Don't** use pure black (#000) for text or shadows; the room is warm.
- **Don't** badge good moves. Only inaccuracy, mistake, and blunder earn a badge.
- **Don't** use monospace for headings, buttons, or prose.
- **Don't** hide a disabled control. It stays visible so the visitor learns the flow.
