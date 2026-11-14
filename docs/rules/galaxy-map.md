# The galaxy map

The main screen, as the original with fan patch 1.51 draws it right after a
new galaxy is made, with the whole galaxy in view. Measured on screenshots
of the original, with the galaxy it made read from its own Lua interface
(see [galaxy.md](galaxy.md)).

## Colours

The screen's colours are FONTS.LBX entry 1 (the game's colours); the frame
picture brings its own first 192, the same.

## Layers

From the bottom up:

1. The sky: STARBG.LBX entry 3, the whole screen.
2. The nebulae: STARBG.LBX entry 6 + 4 × picture + scale, their top left
   corner where the nebula's corner is on the map.
3. The stars, their middle pixel ((width - 1) / 2, (height - 1) / 2) on
   their place: BUFFER0.LBX entry
   148 + 6 × colour + scale + size, frame 0 (blue to brown; each colour has
   six pictures, biggest first). A star's size is 0 to 2, one in three each
   in the measured galaxies, whatever its colour; the savegame keeps it in
   the byte after the star's place. Black holes: BUFFER0.LBX entry
   184 + scale (entry 185 measured for a medium galaxy), 34 × 34, an
   animation of 16 frames.
4. The player's fleets: BUFFER0.LBX entry 208 − scale, frame 0 for a red
   banner (its eight frames are all red, an animation; other banners are
   not measured yet, Hegemon takes the banner's number), its top left corner 5 pixels left of and 1
   above the top right corner of the star's picture on a medium map (measured
   with all three star sizes); 6 left and level on a small map, 6 left and 2
   above on a large one (one star each); the huge map is assumed like the
   large one.
   Fleets sent off, on their way and their courses: see
   [fleets.md](fleets.md).
5. The names of the stars the player knows (at first only their home star),
   in FONTS.LBX font 2 with one pixel between letters, the letters centred
   under the star's middle and their top 14, 11 or 9 pixels below it for a
   star of size 0, 1 or 2 (measured on medium maps). Shade 2 is the banner's
   colour (red: 79), shade 1 a dark one (red: 74), with a dark outline (see
   below).
6. Black, on the left, top and right edges of the frame's opening
   (x 20 and 527, y 20): the map shows from (21, 21) to (526, 421).
7. The frame: BUFFER0.LBX entry 0, whose opening (x 20 to 527, y 20 to 421)
   lets the map through.
8. The side panel's texts (see below).

"Scale" is 0 to 3: the galaxy's map size (small, medium, large, huge; a
cluster galaxy has a large map). With the whole galaxy in view the map is
drawn 506 × 400 pixels from (21, 21), whatever the galaxy's size:
x on the screen = 21 + x × 506 / width, y = 21 + y × 400 / height, rounded
down.

## The outline of texts on the map

Names and the side panel's texts have a dark outline: before the letters,
the whole text is drawn one pixel away in this order, each time over what
was drawn before: left; down right; up left; up; right; down; up right;
down left. Each pixel of shade 1 draws colour 1 there and each of shade 2
colour 2 (this fits 771 of the 773 outline pixels measured).

## The side panel

From the top, in FONTS.LBX font 2 with two pixels between letters, shade 1
in colour 14 and shade 2 in 23, with the outline above. Centred texts are
placed at x = 579 − w / 2, rounded down, where w is the text's width with
the spacing after each letter (the stardate: 580 − w / 2).

| Text | Example | Top |
| --- | --- | --- |
| Stardate, year and tenth | 3500.0 | 29 |
| Treasury | 50 BC | 96 |
| Income a turn | +8 BC | 108 |
| Command points left, and in all | +3 (6) | 181 |
| Food left over | +0 | 255 |
| Freighters left, and in all | +0 (0) | 330 |
| Research project's turns | ~11 turns | 391 |
| Research points a turn, or no project | 10 RP, none (HESTRNGS.LBX message 392) | 403 |

The command points and the freighters are two texts each: the first ends
(with its spacing) at x 578, the second starts at 582 (freighters: 576 and
578). The command points' total in brackets has no outline. These places
were measured with one-digit numbers only. What the numbers are is in
[economy.md](economy.md).

The turns line is only there with a project. It starts with font 2's "@",
which is a wavy line, drawn 3 pixels lower than the rest; the whole text
(with the "@") is placed at x = 584 − (w + 1) / 2. Measured on three turns
of one game: ~11, ~10 and ~9 turns; how the number is worked out is in
[research.md](research.md).

## The Turn button

The Turn button under the side panel, about (547, 444) to (604, 462), ends
the turn. Hegemon also takes Enter and the gamepad's Start.

## Enter Home Star Name

Right after the galaxy is made the original asks for the home star's name
in the same box as the ruler's name (see [ruler-name.md](ruler-name.md)),
its top left corner at (131, 146), Accept at (227, 246), 98 × 28. The title
is HESTRNGS.LBX message 257 ("Enter Home Star Name"), its shades in
colours 12, 24 and 19; the name, offered as the race's home star name from
STARNAME.LBX, in colours 12 and 24.

## Still to do

Wider numbers on the side panel, the Turn button's pressed picture, other players' fleets and fleets in flight, the black holes'
animation, the sky's twinkling, the nebulae on the frame's bottom row (the
original draws a nebula's pixels on row 421 over the frame there), the names
under stars of other sizes on the other maps, zooming and scrolling, and
everything a click on the map does.
