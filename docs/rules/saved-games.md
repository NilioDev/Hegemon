# Saved games: the Load Game window

Measured on fan patch 1.51 under DOSBox, from the main menu, with saved
games in slots 1, 2, 5 and 10 whose names were changed in the file (see
[the file layout](../formats/savegame.md)), and compared pixel for pixel
with Hegemon's window.

## The window

Load Game in the main menu opens GAME.LBX entry 11 (279 × 378) at
(180, 51). It is drawn for the game's colours (FONTS.LBX 1) and shown in
the main menu's colours nearest to them. Under it the main menu shows its
buttons, none lit, but no credits and no version: they stop while the
window is open.

There are ten slots, `SAVE1.GAM` to `SAVE9.GAM` and then the autosave,
`SAVE10.GAM`. A slot is 232 × 29 pixels at x 202, with one every 31 pixels
from y 71. In a slot, from its top:

| What | Where | Font, spacing |
|---|---|---|
| The saved game's name, or "... empty slot ..." | (212, +4) | 2, 2 |
| "Stardate:" | (212, +18) | 1, 1 |
| The stardate, such as 3500.1 | (254, +18) | 1, 1 |
| The date it was saved, such as "Nov 25, 2026" | (312, +18) | 1, 1 |
| The time, such as "02:34" | (367, +18) | 1, 1 |
| A player (GAME.LBX 23), in the screen's own colours | (408, +16) | |

The date and time are the file's, in local time. The text is in colours
254 and 255 of the main menu's palette. The slot loaded last is drawn in
251 and 252, which are brighter. The original remembers that slot in its
settings file (byte 22 of `150.SET`, 0xFF for none). Hegemon remembers it
until it closes.

## Clicking

- A click on a slot loads its game at once. Load and Cancel aren't needed
  for it.
- A click on an empty slot shows a warning, such as "SAVE3.GAM does not
  exist".
- **Load** (GAME.LBX 12 at (217, 388)) loads the slot loaded last. With
  none, it does nothing.
- **Cancel** (13 at (351, 389)) closes the window.

Both buttons show their second frame while held. Loading shows the galaxy
map of the saved game, as Continue does.

## The warning

The box is WARNING.LBX 0 (331 × 191) at (154, 144). It is drawn in its own
colours and shown in the screen's colours nearest to them. Its seven frames
light the red lamps on both sides. They are shown forwards and back
(0, 1, … 6, 5, … 1, 0, …), one every two timer ticks (about a ninth of a
second).

The text is in font 4, spacing 2, centred on x 322, 88 pixels below the
top. It uses shades 3, 137 and 123 of the main menu's colours. A click
anywhere closes the box and goes back to the window.

## Hegemon's own

- Esc or the gamepad's B closes the window.
- The arrow keys, the D-pad or the left stick move the pointer from slot
  to slot, and Enter or A clicks there.
- Any key or gamepad button also closes the warning.

## Not measured yet

- Whether the day of the month gets a leading zero ("Nov 05").
- Whether the time column moves with a longer date.
- The player pictures of a multiplayer game (entries 16 to 19, 24 to 26).
- The message "Loaded config: …" that the fan patch shows after a game is
  loaded.
- The same window in the game, with its Save Game twin (entry 14) and the
  game menu (entry 0).
