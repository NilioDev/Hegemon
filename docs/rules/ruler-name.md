# The ruler's name

Taking a stock race in the race selection (see
[race-select.md](race-select.md)) opens the Enter Ruler Name dialog.
Measured on the original's DOS program with fan patch 1.51 under DOSBox.
Hegemon's drawing matches the captures for the Alkari and the Bulrathi pixel
for pixel, apart from the pointer.

## What is drawn

1. Entry 0 of `RACEOPT.LBX`, the frame with the green grid, without the race
   selection's panel. The screen keeps the race selection's colours: those
   of `RACEOPT.LBX` and, from 128 to 255, those of the taken race's portrait.
2. The dialog, frame 0 of entry 0 of `INBOX.LBX`, at (177, 125), 288 × 151.
   It carries colours 0 to 31 of its own, but they are not put in the
   palette: each is drawn in the nearest colour of the screen (see
   [../formats/images.md](../formats/images.md)). With a different race the
   same dialog comes out in slightly different colours. The Accept button
   is part of the picture; it is at (273, 225), 98 × 28, the size of entry
   1 of `INBOX.LBX`.
3. The title, the only record of entry 156 of `KENTEXT1.LBX` (the English
   texts; see [../formats/text.md](../formats/text.md)): "Enter Ruler
   Name". Font 4 of `FONTS.LBX`, two pixels between letters, at y = 147,
   centred on the dialog: x = 177 + (288 − width) / 2, rounded down. Shades
   1, 2 and 3 in the dialog's colours 10, 26 and 20, mapped like the
   dialog. There is no shadow.
4. The name at (221, 186) in font 3, two pixels between letters, shade 1 in
   the dialog's colour 0 (black) and shade 2 in its colour 20.
5. The cursor, an underscore in the same font and colours, right after the
   name. It blinks: the original redraws it as fast as it can and shows it
   every other time, about 20 to 40 changes a second under DOSBox. Hegemon
   flips it on every tick of the PC timer (about 18 a second).
6. The pointer: picture 0 of entry 10 of `FONTS.LBX`.

## The name

The dialog offers one of the race's eight names in `RACENAME.LBX` (see
[../formats/races.md](../formats/races.md)) at random, a new one every
time it opens.

- A key adds its character at the end. Letters, digits, punctuation and
  spaces are all taken. The first key does not clear the offered name.
- A key adds nothing when the name has 19 characters, or when the name is
  already 100 pixels wide or wider (its width in font 3 with the two pixels
  after every letter). So "Karaaw Hrik" takes two more x's, seven W's fit
  and nineteen i's.
- Backspace takes the last character away. The name may be left empty; the
  original then goes on all the same.
- Enter, or letting go of the mouse button over Accept, takes the name and
  opens the banner colours (see [banner.md](banner.md)). The Accept button
  doesn't change while it is held.
- Esc goes back to the race selection. Esc on the banner colours comes back
  to this dialog with a new name.

**Added in Hegemon:** with a gamepad, A takes the name and B goes back. On
systems with an on-screen keyboard it opens with the dialog.

## Still to do

- A few single key presses in the measurements broke the width rule above
  (after twelve a's, a j and then an i were taken, while two i's were not).
  They may have been keys DOSBox lost; the rule fits every repeated test.
- What the original does with an empty name later in the game.
- What frame 1 of `INBOX.LBX` entry 1, a lit Accept, is for.
