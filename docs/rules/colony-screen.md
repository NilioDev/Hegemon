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

Measured with these: a scientist's research is rounded to the nearest
point for the colony (three Elerian scientists under feudalism, 4.5, make
5 on the screen and on the galaxy map).

A small ship stands at the right end of the production row (about 276, 84)
on every home colony; where it comes from is still to be found.

## Buttons

Return covers (552, 457) to (628, 478). Leaders, Change and Buy are in
the frame too and do nothing yet.

## Still to do

Choosing another planet of the system, the ship at the end of the
production row, moving people between jobs, the buildings and roads in the
landscape, the build queue with Change and Buy, Leaders, the ships in the
sky, the marines at the bottom left, the stars' twinkling and what the
mouse does on the screen.
