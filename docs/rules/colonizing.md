# Colonizing a planet

How a colony ship lands. Measured on version 1.51 of the fan patch: a
colony ship of a new Alkari game sent with the scouts to Sal (a large, poor
ocean planet in the second orbit and a huge barren one in the fifth), with
screenshots of each step.

## Choosing the planet

- When ships with a colony ship reach a star, the original first shows
  their arrival (see [star-system.md](star-system.md)). Close then opens the
  star system window again in the middle of the map, titled "Select planet
  for Colony Ship in %s system" (ESTRINGS.LBX message 462).
- The fleet window of a fleet with a colony ship at a star with a planet to
  settle shows Colonize Planet under Close (see [fleets.md](fleets.md));
  it opens the same window.
- A planet can be settled when it is a planet (not an asteroid belt or a
  gas giant) without a colony. A click on one asks before the ship lands;
  Close leaves the ship where it is. Hegemon also takes Enter, Space or A
  for the first planet that can be settled.

## The question

CONFIRM.LBX entry 0 (313 × 227, with its own colours, drawn in the nearest
of the screen's) at (161, 117):

- The title "Build colony on %s with" (ESTRINGS.LBX message 209, the
  planet's name), font 3 with two pixels between letters, centred on x 316,
  at y 155.
- Six rows from (204, 185), 15 pixels apart, font 3 with two pixels between
  letters, shades 1 and 2 in the box's colours 85 and 91; the values end at
  x 429. The words and values come from ESTRINGS.LBX, split where the
  value goes:
  - Food per farmer (101, or 100 when it is a half), what one farmer grows
    there;
  - Industry per worker (107);
  - Research per scientist (115);
  - Maintenance penalty (108) and Worker penalty (120): +0% on the measured
    planet. Hegemon shows +0% until the penalties are known;
  - Max Population (109).
  The measured values for the Alkari on Sal Prime: 2, 2, 3, +0%, +0%, 5.
- YES (entry 1) at (235, 302) lands the ship; NO (entry 2) at (345, 302)
  goes back to the star system. Hegemon also takes Enter or Y and Esc or N,
  and A and B on a gamepad.

This draws the measured box to the pixel.

## The landing

A full screen in the colony screens' colours (FONTS.LBX palette 2):

- The sky, COLONY2.LBX entry 49, and the planet's landscape, PLANETS.LBX
  entry 3 × climate + the planet's picture, as on the colony screen.
- The ship coming down: COLONY.LBX entry 6 (640 × 480, 73 frames, a frame
  every 1/20 second, each drawn whole). Its colours 240 and 241 are the
  shadow: they darken what is under them to the nearest half as bright
  colour.
- On top, "Colony built on %s" (ESTRINGS.LBX message 229), font 4 with two
  pixels between letters, centred on x 319 at y 10, shades 1, 2 and 3 in
  colours 245, 253 and 252.
- Once the ship is down, five lines from (20, 40), 20 pixels apart, in the
  same font and colours: "%s - %s colony" (message 40, the planet and the
  race, 645 + race), the size and climate, "%d Total Population" (31, the
  planet's most people), "Mineral %s" (374) and the gravity. The words:
  sizes 683, 480, 371, 360, 323; climates 539, 719, 720, 721, 722, 399, 501,
  184, 723, 303; minerals 684, 423, 685, 450, 686; gravity 687, 688, 689.

The measured lines on Sal: "Sal Prime - Alkari colony", "Large Ocean",
"5 Total Population", "Mineral Poor", "Normal G". The landing draws within
a few hundred pixels of the measured screens, most of them the pointer.

Esc did not leave the landing in the original; Hegemon takes a click,
Enter, Space or Esc (A or B) to land the ship at once and then to go on.

## The new colony

- The colony ship is gone and the planet has a colony of one person of the
  player's race, a worker (measured on Sal Prime, where a farmer would have
  fed it; assumed for every planet).
- When it is the player's first colony at the star, the original asks for
  the star's name: the "Enter Star Name" box (HESTRNGS.LBX message 256) at
  (177, 130), with the star's name to change (that it comes only for the
  first colony at a star is assumed).
- Then the new colony's screen opens with "%s just colonized, place your
  orders for future production." (ESTRINGS.LBX message 64) in a message box.
  The measured colony screen is titled "Industrial Colony of Sal Prime";
  Hegemon's says "Colony of", and where the original puts the message box
  is still to be measured.
- After the colony screen the rest of the turn follows.

Not done yet: outpost ships, the gravity and maintenance penalties, and
which colony type the colony screen's title names.
