# The colony screen

What the original shows for a colony, as far as measured with fan patch
1.51 (the Alkari's home colony, Altair Prime, after the first turn).

## When it opens

After the first turn ends, the original shows the human's home colony,
which has nothing to build yet (measured after choosing the first research
project); after the second turn it doesn't show it again. Hegemon can't
build yet, so it shows each colony once. Return goes back
to the galaxy map; Hegemon also takes Esc, Enter and the gamepad's A or B.

## The picture

From the back:

- The sky, COLONY2.LBX entry 49, a 640 × 480 field of stars.
- The landscape, PLANETS.LBX entry climate × 3 + the planet's picture
  (climates counted from toxic 0 to gaia 9). Which of the three pictures a
  planet has is chosen at random when the galaxy is made; in the original's
  savegames each turns up equally often for every kind of planet (1,800
  planets of 26 new games).
- The frame with its panels and buttons, COLPUPS.LBX entry 5.

The colours start from FONTS.LBX entry 2's palette (its
pointers are this screen's), then the landscape's (0 to 79) and the sky's.
Measured: the sky, landscape and frame fit the original pixel for pixel
where nothing is drawn over them (the stars twinkle).

## Buildings and roads

Between the landscape and the frame stand the colony's buildings, joined by
roads (measured on the home colonies of 39 new games):

- A building's picture is a whole 640 × 480 screen in BLDG0.LBX to
  BLDG4.LBX, counted on from one file to the next (360 entries a file):
  36 places for each building, in the order of the fan patch's building
  table (picture 8 the capitol, 21 the marine barracks). Place 0 is at the
  front, 35 at the back; Hegemon draws them from the back.
- Roads are whole screens too, COLROADS.LBX entries, drawn under the
  buildings.
- Colours 240 and 241 in either are shadow: what is under them turns to
  the colour nearest half as bright, counted in the palette's six bits a
  channel, the lowest index winning a tie (measured on 43 colours).
- A home colony is laid out in one of four ways, about as often each
  (11, 7, 10 and 8 of 36 games); Hegemon chooses one at random when the
  game starts. Each also has pictures 2 (domes) and 13 (modules), which
  belong to no building the colony has. As place: picture.

  | Layout | Capitol | Marine barracks | Domes | Modules | Roads |
  | --- | --- | --- | --- | --- | --- |
  | 1 | 10 | 7 | – | 17, 26, 28 | 16, 25, 28, 31, 32, 50, 72 |
  | 2 | 15 | 28 | 1 | 0, 20 | 6, 20, 26, 34, 42, 43, 49, 50, 66, 67, 74 |
  | 3 | 10 | 33 | 6 | 0, 35 | 0, 5, 6, 23, 37, 43, 48, 50, 51, 61, 62, 78 |
  | 4 | 10 | 14 | 24 | 0, 25 | 0, 6, 7, 10, 13, 20, 43, 46, 51, 53, 54, 58, 59 |

  Without a capitol (Unification) three layouts were seen:

  | Layout | Marine barracks | Domes | Modules | Roads |
  | --- | --- | --- | --- | --- |
  | 1 | 7 | – | 0, 18, 28 | 0, 25, 33, 34, 39, 74 |
  | 2 | 6 | 0 | 10, 14 | 7, 13, 14, 42, 51, 59, 78, 79 |
  | 3 | 15 | 28 | 0, 14 | 14, 20, 28, 42, 59, 66 |

  Pictures 39 (star base) and 40 (star fortress) are the same as 13. The
  layout is not in the savegame at a fixed place; how the original picks
  it, and where it puts buildings built later, is still to be measured.

## The texts

- The title, ESTRINGS.LBX message 97 ("%sColony of %s", the first blank
  empty) with the planet's name: font 4 with two pixels between letters,
  centred on x 320 (320 − w / 2), at the top. Shades 1, 2 and 3 in colours
  245, 253 and 252.
- The population, message 424 ("Pop %d,%03d k (%+dk)": the people, the
  thousands grown towards the next one and the growth a turn): font 2 with
  two pixels between letters, ending at x 639 (with its spacing), 3 pixels
  down, in the same colours.

A planet's name is its star's name and HESTRNGS.LBX message 160 ("Prime")
for the first orbit (measured: Altair Prime). For the others Hegemon
assumes a Roman number by the orbit (Altair II for the second); one
measurement of another game showed a home world "Altair II".

## The planets of the system

On the left, one place for each of the star's five orbits, 24 pixels apart
(measured on all thirteen races' home systems, pixel for pixel but for the
stars' twinkling):

- An arrow, COLSYSDI.LBX entry 64, at (6, 31) for every orbit, empty ones
  too. By the colony shown it pulses through the entry's frames (the
  original's pictures caught frames 1 to 5; one a tick is assumed).
- The planet's picture, centred on (22, 38): COLSYSDI.LBX entry 6 + 5 ×
  (climate + 1) + size (climates from toxic 0, sizes from tiny 0), 61 for
  a gas giant and 63 for an asteroid belt. Its left edge is 22 − w / 2 and
  its top 38 − h / 2, rounded down.
- Its text in font 1 with a pixel between letters, at (35, 31), the second
  line 11 pixels down, in colours 245 and 253; the colony shown in 236 and
  238 (measured with the red banner). ESTRINGS.LBX:
  - A planet: message 44, "%s %s- %s\n%s%s": the size (502 T, 604 S, 605
    M, 606 L, 607 H), the climate (539, 719, 720, 721, 722, 399, 501, 184,
    723, 303), the gravity (361 LG, 603 HG, nothing for normal), then the
    minerals (546 U-Poor, 423 Poor, nothing for abundant, 450 Rich, 547
    U-Rich) and an empty blank. "L Barren-" over "U-Poor".
  - A colony: message 41, "%s - %s\n%s%s": the race's name (645 +
    race), the gravity, then the minerals and message 8, " (%d/%d)", the
    people and the most the planet holds. "Mrrshan -" over "Rich (8/12)".
  - A gas giant: message 306, "Gas Giant - uninhabitable", broken at a
    space: "Gas Giant -" over "uninhabitable". Hegemon breaks a line wider
    than 60 pixels at its last space that fits (assumed).
  - An asteroid belt: message 186, "Asteroids".

## The rows at the top

Four rows of COLONY2.LBX icons start at x 128: money at y 32, food at 64,
production at 94 and research at 124. Measured on the home colonies of all
thirteen races after the first turn; each row matches the original pixel
for pixel.

- Icons: food 0, production 1, research 2, money 3; four further on (4 to
  7) the same for ten, which come first. Pollution is 9 (13 for ten).
  Darker ones (8 food, 12 ten food, 11 and 15 money) are what Hegemon
  assumes for food that is missing and for money lost; not measured.
- Places: the row's icons and its empty places are 20 pixels apart, or
  150 ÷ (places − 1), rounded down, when that is less. Eleven coins and a
  gap are 13 apart, so they overlap; the later one is drawn on top.
- Money: the buildings' upkeep, an empty place, then the colony's income
  with halves rounded up as on the galaxy map (Alkari 3 and 8, Gnolam 3 and
  16 as a stack of ten and six coins).
- Food: what the people eat, then an empty place and what is left over
  (Klackons: 8, a place, 1). Cybernetic races eat production too: the
  production they eat follows the food, then an empty place even when
  nothing is left over (Meklars: 4 food, 4 production, then the row is 18
  apart). Lithovores have no food row.
- Production: what is left after the cybernetic upkeep, then a barrel for
  each point that pollution takes (Klackons: ten and two barrels).
- Research: the research points (Alkari 10 with the artifacts, one icon).

The government's icon is COLONY2.LBX entry 19 + the government pick
(feudal 19, dictatorship 21, democracy 23, unification 25) at (310, 32).

The people work in the rows to the right: RACEICON.LBX entry 13 × race + 1
for a farmer, + 3 for a worker and + 5 for a scientist, at x 310 and on 30
apart, at y 62, 92 and 122. Closer together when more than seven don't fit
is assumed.

People are moved between jobs by dragging them. Measured: pressing on a
person picks it up together with all the people to its right in that row.
They follow the pointer, drawn in the job of the row under it, and the
population on the screen leaves them out while they are held. The original
also shows a "Transport Colonists" panel then, for sending them to another
planet. Letting go over another row puts them in that job, and the rows,
the population and the side panel are worked out again. Assumed until
measured: anywhere else they go back. How far apart the held people are
drawn is assumed too (15 pixels, from 8 right and 10 above the pointer).

**Added in Hegemon:** Up and Down (D-pad up and down) choose a row. Space
(A) picks up its last person, Up and Down then choose the row it goes to,
and Space (A) puts it down there.

Measured with these: a scientist's research is rounded to the nearest
point for the colony (three Elerian scientists under feudalism, 4.5, make
5 on the screen and on the galaxy map).

A small ship stands at the right end of the production row (about 276, 84)
on every home colony; where it comes from is still to be found.

## What is built

The box at the top right shows the first of the build list, measured: its
name in font 1 centred on 578 at y 38, "%d turns" (HESTRNGS 226, also for
one turn) from (531, 103) and "made / cost" ending at 626 in font 0, one
pixel between letters, shades 245 and 253, a black shadow. The turns are
what is left over the colony's production a turn, rounded up. With nothing
to build it reads "1 turns" and "0 / 0". Still to do: the bar at its
bottom right, and the box's frame, which is drawn lighter while something
is built.

## Product pictures

The box at the top of the build list shows the picture of the product
under the pointer, and the colony screen's box that of the first of the
build list (measured pixel for pixel on the original):

- Colony, outpost and transport ships and freighters: SHIPS.LBX entries 45,
  46, 47 and 48 at (218, 32). They are drawn for the colours of FONTS.LBX
  entry 1, with the player colours 192 to 223 as two ramps of 16 greys (six
  bits: 5, 9, 13, 17, 20, 24, 28, 32, 36, 40, 43, 47, 51, 55, 59, 63), colour
  1 the darkest of them and 31 white, and shown in the screen's colours
  nearest to those; its unused colours (pure green, such as 224) show the
  screen's own.
- A spy: RACEICON.LBX entry 13 × race + 11 at (230, 42).
- Trade goods: COLONY2.LBX entry 7 at (235, 54); the colony screen's box
  shows none.
- A building: its landscape picture at place 30 (housing: picture 13), its
  shadow darkening what is under it as on the landscape. What is not shadow
  is centred on x 244 (left = 244 − width / 2) with its bottom row at 92.
- In the colony screen's box everything is 334 pixels to the right and 21
  down, a building 335 and 13, and cut off at the box: (522, 15) to (635,
  99).

## Buttons

Return covers (552, 457) to (628, 478), Change (520, 124) to (579, 143)
and Buy (590, 124) to (626, 143); Change opens the build list. Leaders is in
the frame too and does nothing yet.

## Buy

Buy finishes the first of the build list now, for BC. Measured on the
original (fan patch 1.51), with the messages of ESTRINGS:

- With nothing in the list: "You have no product selected to buy." (587).
- With housing or trade goods first: "You cannot buy Housing or Trade
  Goods, as they are settings, not specific items." (568).
- When what is left is no more than the colony makes a turn (assumed from
  the message): "You cannot spend BC's to get this faster than in a single
  turn." (578).
- When the price is more than the treasury: "You cannot afford this
  increase in production. You are short by %d BC." (564), the price less
  the money.
- Otherwise the question "Do you want to spend %d BC to buy %d
  production?" (269) with the price and what is left; Yes pays and fills the
  product up, so the box reads "1 turns" and "cost / cost", No changes
  nothing.
- The price: each point still to make costs 10 BC in the product's first
  tenth, 5 up to its half and 2 after, so 4 BC a point with nothing made.
  Measured on a spy (100) as it was built, 6 a turn: 400, 340, 290, 260,
  230, 200, 170, 140, 110 and 92 BC; on a colony ship (500) 2000, 1940 and
  1880 BC.
- The fan patch's manual: a second Buy in the same turn gives the BC back
  and takes the production away again. What was bought is forgotten when
  the turn ends.

The messages are boxes over the screen; they keep the screen's colours and
are drawn in those nearest to the colours of their own palette (the least
sum of squared 6-bit differences), the text too.

- A message: TEXTBOX.LBX. Its inside is lines of the box's colours 2, 3 and
  4 in turn, from x 140 to 499, starting 11 rows below the box's top
  (earlier rows of the pattern show where the frame leaves room) and ending
  9 rows into the bottom picture. Over them the top (entry 0, 42 rows), the
  sides from the body (1) for as many rows as the text needs, 15 a line
  less one, and the bottom (2, 43 rows), at x 130, the whole box centred
  on the screen's height. The text in font 3, two pixels between letters,
  from x 159, 33 rows below the top and 15 a line, shades 166 and 168,
  broken at spaces to fit 322 pixels. Close (3, pressed in its second
  frame) 16 rows into the bottom at x 288.
- A question: CONFIRM.LBX, the box (0) at (161, 117), Yes (1) at (235, 302)
  and No (2) at (345, 302), each pressed in its second frame. The text in
  font 4, two pixels between letters, each line centred on 317, 17 pixels
  a line with the block's middle at 216, shades 230, 232 and 231, broken to
  fit 230 pixels. Esc does not close it in the original; in Hegemon Esc
  and N answer No and Enter and Y answer Yes.

## The build list

Change shows the colony's build list over the colony (COLBLDG.LBX; its
entry 0 is the frame, with a palette of its own). Measured on the
original:

- Behind the frame the colony's scene is shown in the blue ramp 244 to
  254. A landscape colour (0 to 79) goes by its brightness in six bits a
  channel: on even rows 2 red + green + blue, level 2 below 28, 3 below 57,
  4 below 83, 5 below 116, 6 below 150 and 7 above; odd rows are darker,
  2 red + 2 green + blue, level 0 below 33, 1 below 66, 2 below 96, 3 below
  127, 4 below 158, 5 below 190 and 6 above. The sky and the buildings
  (colours 80 and up) are flat: level 2 on even rows, 0 on odd ones. The
  boxes of the title, (231, 307) to (432, 327), and of the seven lines,
  (206, 331 + 20k) to (460, 347 + 20k), are darker still: each colour the
  one nearest half as bright, as under a building's shadow.
- On the left, in font 4 with two pixels between letters from (13, 20),
  19 pixels a row: Trade Goods, Housing and the buildings the colony can
  build, in the order of the fan patch's prod_format, names alphabetically
  within an order. On the right from (485, 20): Colony Base (when another
  planet of the system is free), Freighter Fleet, Colony Ship, Outpost Ship
  and Transport Ship as their techs are known, the six ship designs on rows
  6 to 11 and Spy on row 13.
- Text colours are the fan patch's font_color_group g0: the left list's
  products are in its "normal" colours (203, 202, 183) until they are in
  the build list, then in its "queued" ones (187, 186, 184), which the right
  list, the title, the lines and the box at the top always use. Shades 1 to
  3 are the dark, base and dim colours; the shadow is black, one pixel to
  the right, below and both.
- The title "Build List for %s" (ESTRINGS 210) and the lines in font 3,
  centred on 332 (x = 332 − width / 2), at 311 and 335 + 20k.
- A product clicked goes at the end of the list, at most seven lines;
  ships, spies, housing and trade goods may be there more than once. The
  line under the pointer reads "delete %s" (ESTRINGS 257) in a frame of
  colour 239, (207, 332 + 20k) to (458, 347 + 20k); a click deletes it.
- The box at the top shows what the pointer is on: its name in font 4 at
  (306, 21), "Cost Here: %d" (242) at (306, 47) and "Build Time: %d" (211)
  at (306, 60) in font 3, the turns being the cost over the colony's
  production rounded up. Housing and trade goods show neither. For a
  building or the colony base "Maint. Cost: %d" (367), its maintenance from
  the building table, comes at (306, 60) and the build time a line lower,
  at 73.
- The box below holds the product's description: the help text of its tech
  (HELP.LBX entry 0, from byte 103 of the tech's record; see
  [text.md](../formats/text.md)). The ships, freighters and the colony base
  have their own techs, the transport ship "transport" and the spy
  "spy_network"; a building its tech_id (assumed: no building could be
  built in the game measured). In font 3, one pixel between
  letters, in the same colours and shadow, from (209, 119), 15 pixels a
  line. Words go on a line while it is at most 249 pixels wide counting 3
  pixels between words (seen: 249 fits, 253 doesn't); every line but the
  last is then spread to 248 pixels by widening the gaps evenly, the
  leftmost a pixel more where it doesn't share out (measured on Spy, Colony
  Base, Freighter Fleet and Outpost Ship). Housing and trade goods have
  none; the ship designs show their details instead, still to come.
- Costs: buildings from the building table, colony, outpost and transport
  ships from lander_ship_cost, freighters and spies from misc_costs.
- Buttons: Cancel (COLBLDG 1) at (493, 447), OK (3) at (560, 447), each
  pressed in its second frame; Refit (4) at (492, 379) and Design (5) at
  (561, 379) are greyed while there is nothing to refit.

## Still to do

Choosing another planet of the system, the ship at the end of the
production row, sending people to other planets, the people walking, the
trees and spires, the star base in the sky, buildings built later,
building what the build list holds, the ship designs and their
details in the build list, Auto Build, Repeat
Build, Refit and Design, Leaders, the ships in the
sky, the marines at the bottom left, the stars' twinkling and what the
mouse does on the screen.
