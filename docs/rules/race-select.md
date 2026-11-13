# Race selection

The Select Race screen follows Accept on the New Game screen (see
[new-game.md](new-game.md)). Measured on the original's DOS program with fan
patch 1.51 under DOSBox. The fan patch brings its own `RACESEL.LBX`
(`150\lbx\RACESEL.LBX`); the numbers below are its entries. Hegemon's
drawing matches the captures of all thirteen races pixel for pixel.

## What is drawn

1. Entry 0 of `RACEOPT.LBX`, the frame with the green grid, with colours 0
   to 127.
2. The portrait of the highlighted race at (54, 63): entries 15 to 27 for
   the thirteen races, 28 for Custom. Each carries colours 128 to 255.
   Custom's portrait has see-through parts where the grid shows.
3. The panel, entry 0, at (48, 58). Its frame lies over the portrait's
   edges.
4. The title, entry 33, at (366, 52).
5. Fourteen buttons in two columns at x = 351 and 477, from y = 90 down in
   steps of 48. Left: Alkari, Bulrathi, Darloks, Elerians, Gnolams, Humans,
   Klackons (entries 1 to 7). Right: Meklars, Mrrshan, Psilons, Sakkra,
   Silicoids, Trilarians, Custom (8 to 14). Frame 0 is the button; frame 1,
   only the yellow text, goes over it on the highlighted one.
6. The race's picks as text under the portrait (none for Custom), see
   below.
7. The pointer: picture 0 of entry 10 of `FONTS.LBX`.

## The text

The text names the race's picks, as [../formats/races.md](../formats/races.md)
describes, joined by ", ". For example: `Food Production: +1, Industrial
Production: +1, Large Home World, Uncreative, Unification`.

It is drawn in font 2 of `FONTS.LBX` with two pixels between letters: shade
1 in colour 120, shade 2 in colour 124, over a black shadow one pixel to the
right, one below and one right below every pixel of a letter. The first line
starts at (50, 391), and the next lines follow 13 pixels lower. The lines
break at spaces, with as many words on each as fit in 295 pixels. Any width
from 293 to 299 breaks the thirteen texts the same way.

## The mouse and the keys

- The highlighted race is the last button the pointer was over. Pointing
  away from the buttons keeps it. When the screen opens, it is the button
  under the pointer.
- A race is taken when the mouse button is let go over its button. Taking a
  race opens the ruler's name (see [ruler-name.md](ruler-name.md)), and
  Custom opens the custom race screen.
- Esc goes back to the New Game screen, with its settings as they were
  accepted.

**Added in Hegemon:** with a gamepad, the D-pad (up and down, left and right
for the other column) or the left stick moves the highlight, A takes it and
B goes back.

## Still to do

- Which race is highlighted when the screen opens with the pointer away from
  the buttons. Hegemon takes Alkari.
- The custom race screen.
- A right click probably shows a help text, as on the New Game screen.
