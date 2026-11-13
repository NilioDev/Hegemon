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
3. The stars, centred on their place: BUFFER0.LBX entry
   148 + 6 × colour + scale + size, frame 0 (blue to brown; each colour has
   six pictures, biggest first). A star's size is 0 to 2, one in three each
   in the measured galaxies, whatever its colour.
4. The names of the stars the player knows (at first only their home star),
   in FONTS.LBX font 2 with one pixel between letters, the letters centred
   under the star's middle and their top 11 pixels below it. Shade 2 is the
   banner's colour (red: 79), shade 1 a dark one (red: 74), with a dark
   outline around the letters (colours 1 and 2).
5. Black, on the left, top and right edges of the frame's opening
   (x 20 and 527, y 20): the map shows from (21, 21) to (526, 421).
6. The frame: BUFFER0.LBX entry 0, whose opening (x 20 to 527, y 20 to 421)
   lets the map through.

"Scale" is 0 to 3: the galaxy's map size (small, medium, large, huge; a
cluster galaxy has a large map). With the whole galaxy in view the map is
drawn 506 × 400 pixels from (21, 21), whatever the galaxy's size:
x on the screen = 21 + x × 506 / width, y = 21 + y × 400 / height, rounded
down.

## Enter Home Star Name

Right after the galaxy is made the original asks for the home star's name
in the same box as the ruler's name (see [ruler-name.md](ruler-name.md)),
its top left corner at (131, 146), Accept at (227, 246), 98 × 28. The title
is HESTRNGS.LBX message 257 ("Enter Home Star Name"), its shades in
colours 12, 24 and 19; the name, offered as the race's home star name from
STARNAME.LBX, in colours 12 and 24.

## Still to do

The side panel (stardate, money, research, food, ships, the Turn button),
the fleets at the home stars, black holes, the sky's twinkling, the exact
colours of the names' outline, zooming and scrolling, and everything a
click on the map does.
