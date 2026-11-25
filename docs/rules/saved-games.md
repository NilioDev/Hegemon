# Saved games: the game menu, Load Game and Save Game

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
| The date it was saved, such as "Nov 25, 2026", centred on 337.5 | (+18) | 1, 1 |
| The time, such as "02:34", 4 pixels after the date | (+18) | 1, 1 |
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

## Over the galaxy map

The GAME button above the map (x 250 to 309, y 2 to 19) opens the game
menu, GAME.LBX 0 at (144, 25), in the game's own colours. Its buttons are
entries 1 to 6, the second frame while held:

| Button | Where | What it does |
|---|---|---|
| Save Game | (184, 68) | The Save Game window |
| Load Game | (291, 68) | The Load Game window |
| New Game | (184, 113) | Asks "Do you wish to abandon this game and start a new one?"; Yes opens the New Game screen |
| Quit Game | (291, 113) | Asks "Do you wish to leave …?"; Yes quits |
| Settings | (184, 332) | Not built yet |
| Return | (295, 332) | Closes the menu |

The questions use the Yes/No box of the colony screen (CONFIRM.LBX at
(161, 117)). Hegemon names itself in the Quit question. The music and sound
volumes are GAME.LBX 7, a bar of ten lit lamps. Only as many lamps as the
volume are drawn, at (206, 219) and (206, 241). Hegemon plays no sound yet,
so it shows 5 of 10, the original's setting in the measured games. Esc
closes the menu. The original's Esc on the map does nothing.

Over the map, the Load Game and Save Game windows (GAME.LBX 11 and 14) are
at (144, 25). Every place above moves with the window, 36 left and 26 up.
The text is in the game's colours 86 and 91, and 89 and 126 for the slot
used last. The player picture is entry 16. In the Save window the name is
4 pixels further right, and the rest of a slot's text 2 pixels. Its Save
button is entry 15 at (181, 363), and Cancel is at (315, 363). Cancel, and
Esc, close the window and the menu, back to the map. Continue doesn't change
the slot used last; loading or saving does.

## Saving

- A click on a slot starts typing a name there, with a cursor. The name
  begins with what the slot shows, even the words "... empty slot ..."
  (Backspace takes them away).
- A key adds its character only while the name is narrower than 200
  pixels. Measured, the limit is between 194 and 203.
- Enter, or Save, writes the game in that slot. Both windows close, back to
  the map.
- Esc closes the window unsaved.

The original shows the cursor every other time it redraws the screen, and
sometimes the cursor ends up in the saved name ("Test_"). Hegemon
leaves it out. The first time a window opens in a game, the original also
leaves out the month of every date. Hegemon always shows it.

Hegemon writes what it keeps of a game in the [original's
layout](../formats/savegame.md) and leaves the rest zero. Leaders,
diplomacy and the like aren't kept yet, so the original may not load a
game Hegemon saved. Hegemon writes to `SAVEn.GAM.part` first and then
renames it, so a failed save keeps the old game.

## Hegemon's own

- Esc or the gamepad's Back opens the game menu from the map.
- The arrow keys, the D-pad or the left stick move the pointer from button
  to button in the game menu, and Enter or A presses there.
- Esc or the gamepad's B closes the window.
- In the Save window, A on a slot saves at once under the name the slot
  shows.
- The arrow keys, the D-pad or the left stick move the pointer from slot
  to slot, and Enter or A clicks there.
- Any key or gamepad button also closes the warning.

## Not measured yet

- Whether the day of the month gets a leading zero ("Nov 05").
- Whether the time column moves with a longer date.
- The player pictures of a multiplayer game (entries 16 to 19, 24 to 26).
- The message "Loaded config: …" that the fan patch shows after a game is
  loaded.
- Settings (entry 8) and changing the volumes.
- How the GAME button looks while held.
