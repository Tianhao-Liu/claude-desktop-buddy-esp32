# Vertical Swipe to Cycle displayMode — Design

**Date:** 2026-04-21
**Status:** Approved

## Background

The device currently cycles top-level display modes (Normal HUD, Pet
stats, Info pages) via Key1 short-press. Touch gestures are partially
implemented: in *clock* mode (Normal + idle + RTC synced) a horizontal
swipe cycles the ASCII species; everywhere else, tap actions fire on
`justPressed`. No release-based classification exists outside clock mode.

This spec adds **vertical swipe to cycle through every page** of the
three top-level modes (Normal, Pet × 2 sub-pages, Info × 6 sub-pages —
nine pages total), giving the 1.75C round panel (where buttons are less
discoverable) a first-class gesture.

> **Revision 2026-04-21:** initial design had swipe cycle only the three
> top-level `displayMode` values (matching Key1). Revised to swipe through
> all 9 pages as a flat list — Key1 remains the coarse mode jumper.

## Goals

- Swipe up / down cycles through all 9 pages as a flat list:
  `Normal → Pet 1/2 → Pet 2/2 → Info 1/6 → … → Info 6/6 → Normal`
- Key1 short-press keeps the coarse 3-mode cycle (Normal / Pet / Info)
- BtnB short-press and Pet/Info top-right tap keep sub-page cycling
  within the current mode (unchanged)
- Existing horizontal swipe-species (clock) keeps working
- Existing tap semantics (buddy-heart, transcript scroll) keep working
- Approval prompt and overlay menus remain gesture-safe (no accidental
  page switches during a decision)

## Out of Scope

- Horizontal swipe in non-clock modes (no new sub-page cycle gesture)
- New haptic feedback, animations, or transitions between modes
- Touch gestures in overlays (menu / settings / reset) beyond the
  existing row-tap

---

## Gesture Classification

All touch handling inside `DISP_NORMAL` / `DISP_PET` / `DISP_INFO` moves
from `justPressed` to `justReleased`, with a press-time snapshot already
recorded in `_tpStartX` / `_tpStartY` / `_tpStartMs`.

On release, compute:

```
dx = tp.x - _tpStartX
dy = tp.y - _tpStartY
dt = millis() - _tpStartMs
```

Then classify **in this order** (first match wins):

| Class | Condition | Action |
|---|---|---|
| Vertical swipe | `abs(dy) ≥ 40 && abs(dy) > 2·abs(dx) && dt < 500` | Advance one step in the flat 9-page cycle: `dy < 0` → next, `dy > 0` → previous. Beep `(1800, 30)`. See **Page Cycle** below. |
| Horizontal swipe (clock only) | `tpClocking && abs(dx) ≥ 40 && abs(dx) > 2·abs(dy) && dt < 500` | Existing `nextPet()` / `prevPet()` + playful window (unchanged logic) |
| Stationary tap | `abs(dx) < 12 && abs(dy) < 12 && dt < 800` | Route by `_tpStartX` / `_tpStartY` to the original region-specific tap action |

Threshold values match existing code (`40` / `12` / `500` / `800` — from
the clock-mode swipe already shipped in commit `284c85a`).

---

## Scope & Suppression

Vertical swipe fires only when **all** of these hold:

- `displayMode` ∈ `{DISP_NORMAL, DISP_PET, DISP_INFO}`
- not `napping` and not `screenOff`
- not `inPrompt` (approval screen)
- not `menuOpen || settingsOpen || resetOpen`

Blocked contexts keep their current `justPressed`-based tap handling
unchanged:

- **Approval screen**: tap upper / lower half = approve / deny, immediate
- **Menu / Settings / Reset**: row-tap select + confirm, immediate

---

## Page Cycle

Swipe cycles through every page as a flat list:

```
Normal ──▶ Pet 1/2 ──▶ Pet 2/2 ──▶ Info 1/6 ──▶ Info 2/6 ──▶ … ──▶ Info 6/6
   ▲                                                                    │
   └────────────────────────── up wraps ────────────────────────────────┘

(down reverses — every arrow.)
```

Two helpers own this logic, one file-scope in `main.cpp`:

```cpp
static void swipeNextPage() {
  if (displayMode == DISP_NORMAL) {
    displayMode = DISP_PET;  petPage = 0;                  applyDisplayMode();
  } else if (displayMode == DISP_PET) {
    if (petPage + 1 < PET_PAGES) { petPage++;              applyDisplayMode(); }
    else { displayMode = DISP_INFO; infoPage = 0;          applyDisplayMode(); }
  } else { /* DISP_INFO */
    if (infoPage + 1 < INFO_PAGES) { infoPage++; }
    else { displayMode = DISP_NORMAL;                      applyDisplayMode(); }
  }
}
static void swipePrevPage() { /* mirror */ }
```

`applyDisplayMode()` fires on mode transitions (matches existing Key1
behaviour) and Pet sub-page transitions (matches existing BtnB behaviour);
it is skipped for Info sub-page transitions (also matches existing BtnB).
The asymmetry is preserved because Pet's `drawPet()` leaves the top 70 px
untouched whereas Info's `drawInfo()` clears its whole region.

Key1 short-press is **not** changed — it keeps the coarse 3-mode cycle
(`displayMode = (displayMode + 1) % DISP_COUNT`). Gesture = fine-grained
navigation; button = coarse navigation. Mental model: Cmd+Tab vs Tab.

BtnB short-press and the Pet/Info top-right corner tap are also
**unchanged** — they cycle sub-page within the current mode.

---

## Tap Region Routing (on release)

When classified as stationary tap, use `_tpStartX` / `_tpStartY` (the
press-start position, not the release position) to decide which action
runs. This preserves the current UX: a user targeting a specific region
and lifting their finger there gets the same action they would have
gotten before.

| Mode | Region (from `_tpStart`) | Action |
|---|---|---|
| `DISP_INFO` | top-right `(W-60, 0, 60, 70)` | next info page + beep |
| `DISP_PET` | top-right `(W-60, 0, 60, 70)` | next pet page + beep + `applyDisplayMode()` |
| `DISP_NORMAL` (HUD, not clocking) | buddy body `(12, 20, W-24, 110)` | heart reaction + playful window |
| `DISP_NORMAL` (HUD, not clocking) | bottom strip `(0, H-32, W, 32)` | transcript scroll back |
| `DISP_NORMAL` (clocking) | upper buddy region `_tpStartY < 130` | heart reaction + playful window |

Order inside the stationary-tap branch: most specific regions first
(e.g. approval, menu, Pet / Info corners), then HUD regions.

---

## File-by-File Changes

### `src/main.cpp` — `loop()` touch block

- Delete the current `else if (displayMode == DISP_INFO && tap(...))` …
  `else if (displayMode == DISP_NORMAL && !tpClocking && tap(...))`
  chain that relies on `justPressed`.
- Extend the existing `if (tpClocking && tp.justReleased)` block into
  the superset "tp.justReleased while not in a suppressed context". The
  body becomes the classification table above.
- `_tpStartX/Y/Ms` recording on `justPressed` stays as-is.
- `inPrompt`, menu / settings / reset row-tap blocks stay on
  `justPressed` — not touched.

No other files change.

---

## Open Items

None — all thresholds, regions, and cycle order are inherited from
existing code. Behaviour is fully specified by the classification table
and the Tap Region Routing table.
