# Main menu

Measured on the original's DOS program under DOSBox, by comparing captures
of its screen with pictures from `MAINMENU.LBX`.

## What is drawn

1. Entry 1, the full picture behind the menu, with its 256 colours.
2. Entry 2 on top of it: the panel on the right, with all six buttons in
   their normal look. Its own 256 colours replace those of entry 1 (they are
   the same for the parts that show).
3. Each button in its current look, at x = 415 and, from the top, y = 172,
   195, 217, 240, 262 and 285.

The buttons are, in order: Continue, Load Game, New Game, Multi Player, Hall
of Fame and Quit Game. Each has three pictures in a row, starting at entry 3:
normal, highlighted and greyed out (entries 3 to 5 for Continue, 6 to 8 for
Load Game, and so on up to 18 to 20 for Quit Game). They are 153 pixels wide
and 22 or 23 high.

With these steps Hegemon's screen matches the original pixel for pixel,
except for what is listed under *Still to do*.

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

- The credits that scroll up on the left, and the version text at the bottom
  right; both need the original's fonts.
- A few small lights on the right edge of the frame that change over time.
- The original's mouse pointer: a small gun turret. It is not one of the
  pictures in the LBX archives.
- What each button leads to. Quit Game closes Hegemon for now.
