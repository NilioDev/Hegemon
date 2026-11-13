# Main menu

Measured on the original's DOS program with fan patch 1.51 (`ORION150.EXE`)
under DOSBox, by comparing captures of its screen with pictures from
`MAINMENU.LBX`.

## What is drawn

1. Entry 1, the full picture behind the menu, with its 256 colours.
2. Entry 2 on top of it: the panel on the right, with all six buttons in
   their normal look. Its own 256 colours replace those of entry 1 (they are
   the same for the parts that show).
3. The credits, the mod name and the version text, see below.
4. Each button in its current look, at x = 415 and, from the top, y = 172,
   195, 217, 240, 262 and 285.

The buttons are, in order: Continue, Load Game, New Game, Multi Player, Hall
of Fame and Quit Game. Each has three pictures in a row, starting at entry 3:
normal, highlighted and greyed out (entries 3 to 5 for Continue, 6 to 8 for
Load Game, and so on up to 18 to 20 for Quit Game). They are 153 pixels wide
and 22 or 23 high.

With these steps Hegemon's screen matches the original pixel for pixel,
except for what is listed under *Still to do*.

## The credits

The lines of `CREDITS.LBX` (see [../formats/text.md](../formats/text.md))
scroll up through a window on the left of the screen, in font 3 of
`FONTS.LBX` (see [../formats/fonts.md](../formats/fonts.md)).

- **Layout.** Line `i` (from 0) starts at y = 456 + 14 × `i` − *offset*. The
  left column starts at x = 66: its text, a space, and then as many dots as
  fit while the whole stays at most 126 pixels wide. The right column starts
  at x = 194.
- **Colours.** Shade 1 of the font is drawn in colour 213, shade 2 in 220,
  with a black (colour 0) outline one pixel all round every pixel.
- **Window.** Only rows 179 to 422 change. Each line is drawn on its own:
  its outline never reaches above the line's top, nor left of x = 66.
- **Fading.** Lines near the edges of the window are drawn darker, by where
  their top is:

  | Top | Shade 1 | Shade 2 |
  | --- | --- | --- |
  | up to 170 | 212 | 211 |
  | 172 and 173 | 212 | 213 |
  | 174 to 177 | 212 | 215 |
  | 178 and 179 | 212 | 217 |
  | 180 to 409 | 213 | 220 |
  | 410 and 411 | 211 | 219 |
  | 412 to 415 | 211 | 217 |
  | 416 to 419 | 211 | 215 |
  | 420 and down | 211 | 213 |

- **Timing.** The offset starts at 0 and grows by 2 pixels every third tick
  of the PC timer (1,193,182 Hz divided by 65,536, about 18.2 ticks a
  second), so the text moves about 12 pixels a second.
- **Starting again.** When the last line's top would come above y = 180, the
  offset goes back to 0, so the credits start again from below the window.

Measured by matching 80 captures of the original against Hegemon's drawing:
all of them match pixel for pixel, apart from frames that the capture took
while the original was still drawing them, where a few lines show a colour
from one step earlier.

## The version text and the mod name

Two texts placed by the fan patch's `format version` and `format mod_name`
settings (see [../formats/config.md](../formats/config.md)): a template, a
font (0 to 5 of `FONTS.LBX`), an alignment (`l`, `r` or `c`) and x and y.
Each can also be set alone, as in `format version y = 420;`. In the
template `$VERSION$` stands for `1.51` and `$MOD$` for the name of the first
mod loaded. Without such settings the fan patch uses
`"Version: $VERSION$" 2 r 590 415` and `"Config: $MOD$" 2 r 590 400`; an
empty template draws nothing.

The text's top is at y. Right-aligned, its last pixel column is x − 1: the
standard mod's `"Version $VERSION$" 1 r 541 433` starts `Version 1.51` at
x = 492. Centred texts are placed at x minus half their width; that has not
been measured. Shade 1 is drawn in colour 208, shade 2 in 215, with no
shadow (measured in font 1).

The unpatched 1.31 shows `Version 1.31` in the same place as the standard
mod.

## Which buttons can be used

- **Continue** is greyed out unless the autosave `SAVE10.GAM` is there.
- **Load Game** is greyed out unless there is a game in one of the slots
  `SAVE1.GAM` to `SAVE9.GAM`.
- The other buttons can always be used.

Measured with no savegames (both greyed out), with only `SAVE10.GAM`
(Continue usable) and with only `SAVE3.GAM` (Load Game usable).

## The pointer

A button that can be used is drawn highlighted while the mouse pointer is
over it; a greyed-out button stays greyed out. Holding the mouse button down
does not change the look. The arrow keys do not move a highlight in the
original.

**Added in Hegemon:** with a gamepad or the keyboard, Up and Down move the
highlight from one usable button to the next (skipping greyed-out ones), and
A or Enter takes it. This uses the original's own highlighted pictures.

## Still to do

- A few small lights on the right edge of the frame that change over time.
- Four pixels of the background just left of the version text, and two short
  stretches of the rightmost column (x = 639, rows 136 to 143 and 336 to
  345), are black in the original.
- The original's mouse pointer: a small gun turret. It is not one of the
  pictures in the LBX archives.
- What each button leads to. Quit Game closes Hegemon for now.
