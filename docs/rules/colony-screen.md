# The colony screen

What the original shows for a colony, as far as measured with fan patch
1.51 (the Alkari's home colony, Altair Prime, after the first turn).

## When it opens

After a turn ends, the original shows the human's home colony when it has
nothing to build (measured after choosing the first research project).
Hegemon can't build yet, so it shows it after every turn. Return goes back
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

## Buttons

Return covers (552, 457) to (628, 478). Leaders, Change and Buy are in
the frame too and do nothing yet.

## Still to do

The planets of the system on the left, the rows of food, production,
research and money, the people at work, the buildings and roads in the
landscape, the build queue with Change and Buy, Leaders, the ships in the
sky, the marines at the bottom left, the stars' twinkling and what the
mouse does on the screen.
