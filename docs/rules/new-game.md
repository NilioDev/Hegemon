# New Game screen

Measured on the original's DOS program with fan patch 1.51 (`ORION150.EXE`)
under DOSBox, by comparing captures of its screen with pictures from the
archives. The fan patch brings its own `NEWGAME.LBX` (`150\lbx\NEWGAME.LBX`,
named in `ORION2.CFG`); the numbers below are its entries. Hegemon's drawing
matches the captures pixel for pixel.

It opens from the main menu's New Game button (or its N key).

## What is drawn

1. Entry 0 of `RACEOPT.LBX`, the frame with the green grid, with colours 0
   to 127.
2. Entry 31 of `NEWGAME.LBX`, the panel, at (15, 5).
3. One picture of 65 by 65 pixels for each of the five settings, with the
   text of its value under it (see below).
4. The three switches, each its entry's frame 0, with frame 1 (only the lit
   lamp) over it when the switch is on: Tactical Combat (entry 26) at
   (395, 264), Random Events (27) at (395, 300) and Antarans Attack (28) at
   (395, 335).
5. Cancel (entry 29) at (115, 391) and Accept (entry 30) at (433, 392), each
   frame 0; frame 1 over it is the pressed look.
6. The pointer: picture 0 of entry 10 of `FONTS.LBX`, the same turret as in
   the main menu in this screen's colours.

Entries 1 to 3 carry colours 128 to 255, the same in each; one of them is
always on screen as the galaxy age picture.

## The settings

A click on a picture, or on the box with its text, steps to the next value;
after the last comes the first again. The values in the order a click goes
through them, with their pictures:

| Setting | Picture at | Values (entries) |
| --- | --- | --- |
| Difficulty | (136, 124) | Tutor (4), Easy (5), Average (6), Hard (7), Impossible (8) |
| Galaxy Size | (291, 124) | Small (9), Medium (10), Large (11), Cluster (12), Huge (13) |
| Galaxy Age | (446, 124) | Mineral Rich (1), Average (2), Organic Rich (3) |
| Players | (136, 264) | 2 Players to 8 Players (14 to 20), Random (21) |
| Tech Level | (291, 264) | Pre-warp (22), Average (23), Post-warp (24), Advanced (25) |

The text is drawn in font 2 of `FONTS.LBX` with two pixels between letters,
shade 1 in colour 121 and shade 2 in 123, and no shadow. Its top is at
y = 214 in the upper row and y = 359 in the lower one. It is centred on
x = 169, 325 and 481 for the three columns: it starts at that x less half
its width (rounded down), where the width runs to its last pixel. The boxes
around the texts are 107 by 26 pixels, from 53 left of that centre and 8
above the text's top. A click in the gap between a picture and its box does
nothing.

The first time, the fan patch shows Tutor, Medium, Average, 5 Players and
Average, with all three switches on.

## The mouse and the keys

- A switch flips as soon as the mouse button goes down on it.
- A picture steps on when the mouse button is let go over it (or over its
  box). Pressed on a picture and let go elsewhere, nothing happens.
- Cancel and Accept are taken when the mouse button is let go over them.
  While it is held down, the button under the pointer looks pressed: press
  on Accept, move to Cancel and let go, and it is Cancel.
- No button lights up just because the pointer is over it.
- Esc is Cancel. Enter is a click where the pointer is.
- Cancel goes back to the main menu and forgets the changes: the screen
  shows the old settings the next time. Accept goes on to choosing a race
  (see [race-select.md](race-select.md)).

**Added in Hegemon:** with a gamepad, the D-pad or the left stick moves the
pointer from one item to the next (the five pictures, the three switches,
Cancel and Accept), A clicks where it is and B is Cancel.

## Still to do

- A right click shows a help text for the item under the pointer, over a
  darkened screen (from `HELP.LBX`).
- The original keeps the settings in `150.SET` from one session to the
  next; when it writes them has not been measured.
- The texts come from the original's string tables; Hegemon has them in
  English only for now.
- The other entries of `NEWGAME.LBX`: 0 (two green panels) and 32 (Net Game
  Name) belong to the multiplayer screens.
