# The Fleets screen

Measured on fan patch 1.51 under DOSBox, in the Alkari's game at stardate
3500.1, with two scouts and a colony ship at Altair. Three of its ships were
also edited in the saved game to see what the numbers come from. Hegemon's
screen was compared pixel for pixel with the original's; only the pointer
and the animated marks differ. What wasn't seen is marked as assumed.

## Opening it

The FLEETS button under the galaxy map opens the screen (its plate at about
x 162 to 237, y 436 to 473). Hegemon also opens it with F, or the gamepad's
right stick button. Return, Esc or B goes back to the map.

## The picture

The screen is FLEET.LBX entry 0 in the game's colours (FONTS.LBX entry 1),
with the pointers of the galaxy map. The ships' colours are those of the
player's banner (SHIPS.LBX).

| Entry | What | Where | Frames |
|---|---|---|---|
| 1 | The scroll bar's track | (605, 86) | |
| 2, 5 | The arrows above and below the grid | (606, 59), (605, 325) | held in the second (assumed) |
| 3, 4 | The arrows beside the bar under the map | (19, 249), (283, 249) | the same |
| 6, 7 | All, Relocate | (348, 380), (441, 380) | |
| 8, 15 | Scrap, and Scrap greyed while no ship is lit | (549, 380) | |
| 9, 10 | Support, Combat | (425, 435), (487, 435) | on in the second |
| 12 | Return | (556, 430) | |
| 16 | Leaders, greyed while there are none | (342, 430) | |
| 17 | A lit cell | the cell's corner, 1 higher | |
| 18 | The frame round the ship clicked last | 1 left of and 3 above the cell's corner | |
| 19 to 26 | A fleet on the small map, by banner colour | 1 right of and 10 above its place | 11, animated |
| 34 to 41 | A star with a colony, by banner colour | centred on the star | 5, animated |
| 42 | A star without one | | 5 |
| 44 | A black hole | | 5 |

The scroll bar's track (colour 161, x 607 to 616, y 88 to 318) is lit in
columns of colours 165, 165, 164, 164, 163, 163, 162, 162, 161 and 161 when
the whole fleet fits. Hegemon shortens it for a longer fleet (assumed).

## The fleet

The screen shows one of the player's fleets, the first at first; the arrows
beside the bar go to the others. The bar says what the fleet holds,
"ALKARI FLEET: 1 Colony Ship, 2 Frigates": HESTRNGS.LBX 305 ("%s Fleet: ")
with the race's name, in capitals, then "%d Colony Ship, " and the like
(258 to 263) for the outpost ships, transports and colony ships, then the
warships by hull size with the names at the end of TECHNAME.LBX's first
entry, the last comma left off. It is font 1, centred on 166 at y 255, in
colours 73 and 80 with a black outline all round. The order of the kinds
is assumed, but for the colony ship before the frigates.

The grid has 4 columns and 5 rows of cells, 62 apart from x 345 and 60
apart from y 55, each with its ship's picture 5 right of and 4 below its
corner, in the order of the game's list of ships. Support and Combat show
the colony, outpost and transport ships and the others. One of the two
always stays on: putting out the second puts the first back on.

- At first no ship is lit and the panel is empty.
- A click on a ship lights it, or puts it out if it was lit, and frames it.
  The panel describes the framed ship.
- All lights every ship shown.
- Scrap is greyed until a ship is lit.

Relocate, Scrap and Leaders do nothing in Hegemon yet.

## The panel

Its text is in colours 111 and 116. For a warship:

| What | Where | Font, spacing |
|---|---|---|
| The design's name, as "Scout" | (18, 287) | 3, 2 |
| "Green Crew (1 EP)": the crew's level (HESTRNGS.LBX 138 to 142) and its experience | (18, 301) | 2, 1 |
| The shield, as "No Shield" | (18, 315) | 2, 1 |
| "Beam Attack:" and its value, as "+25" | (18, 329), ending at 134 | 2, 1 |
| "Beam Defense:" and its value, as "+110" | (173, 329), ending at 308 | 2, 1 |
| "Weapons:", "Specials:" | (18, 357), (173, 357) | 2, 1 |
| The weapons, as "1 Laser Cannon (F)", and the specials | from (23, 370) and (188, 370) | 1, 1 |

The values are assumed to end where they do (right-aligned), and the lists
to go down 10 pixels a line. "(F)" is a forward arc; Hegemon writes "FX",
"B", "BX" and "360" for the others (assumed).

- The beam attack is the computer's bonus (the `computer` table) and the
  race's ship attack. An electronic computer gave +25, an optronic one +50.
- The beam defense is the race's ship defense and 5 for each point of the
  design's combat speed: +110 at speed 12, +150 at speed 20.
- Both add the crew's bonus in the `crew_data` table (assumed: the Alkari's
  were green).
- A crew's experience grows by one a turn (`crew_no_turn_xp`); the
  measured ships had 1 EP after the first turn. The saved game keeps it at
  byte 114 of a ship. The original keeps the crew's level apart: an edit to
  572 EP still showed "Green Crew". Hegemon takes the last `crew_data` row
  whose experience the crew has (assumed).

For a colony ship the panel holds its help text (HELP.LBX, as in the build
list) in font 3 with 2 pixels between letters, from x 18, 15 pixels a line,
centred on y 374, its lines at most 300 pixels wide (measured: between 281
and 302). Outpost ships and transports are assumed to do the same.

## The small map

Every star is a dot centred on (15 + x × 305 / width, 52 + y × 182 /
height), rounded down. The player's fleets have their mark there too.
Hegemon draws the first frame of the animated marks.

## Hegemon's own

- The arrow keys or the D-pad move the pointer over the cells and buttons,
  and Enter, Space or A click there.
- Tab and Shift+Tab, or the shoulder buttons, go to the next or previous
  fleet; Page Up, Page Down and the mouse wheel move the grid.

## Not measured yet

- What Relocate, Scrap and Leaders do, and the scroll bar of a long fleet.
- When a crew's level rises, and the crew's bonuses.
- How the marks on the small map blink.
