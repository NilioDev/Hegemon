# The Colonies screen

Measured on fan patch 1.51 under DOSBox, in the Alkari's game at stardate
3500.1 with one colony (Altair II). Hegemon's screen was compared pixel for
pixel with the original's. Everything that one colony can't show is
marked as assumed.

## Opening it

The COLONIES button under the galaxy map opens the screen. It is the
button's plate at about x 6 to 83, y 436 to 473. Hegemon also opens it
with C, or the gamepad's X. Return, or Esc, goes back to the map.

## The picture

The screen is COLSUM.LBX entry 0 (640 × 480), with its own palette for all
256 colours. The pointers are FONTS.LBX entry 2's, as in the colony screen.
Where the small map is, the picture is see-through: the original has black
there, under the faint stars that the picture draws.

Return is entry 1 at (531, 445). It is always drawn over the darker one in
the picture, and its second frame shows while it is held.

The arrows beside the rows are entries 2 (up, at (619, 15)) and 3 (down, at
(619, 316)). Their second frames show while they are held (assumed).

The sort buttons are entries 4 to 10, in the bottom bar. The one chosen
shows its second frame (Name, at first).

| Button | Where | Size |
|---|---|---|
| Name | (89, 446) | 50 × 24 |
| Population | (140, 446) | 78 × 24 |
| Food | (219, 446) | 42 × 24 |
| Industry | (262, 446) | 63 × 24 |
| Science | (326, 446) | 66 × 24 |
| Producing | (393, 446) | 86 × 24 |
| BC | (480, 446) | 36 × 24 |

## The rows

Ten rows show at a time, one every 31 pixels from y 35. Within a row, its
top is 36 + 31 × row:

| What | Where | Font, spacing, colours |
|---|---|---|
| The planet's name, such as "Altair II" | (12, +7) | 2, 2; 244 and 133, a black shadow right, below and diagonally |
| Farmers, workers, scientists (RACEICON.LBX 13 × race + 1, 3 and 5) | from x 101, 236 and 378, at +2 | |
| What is built, "Colony Base - 34t" | (512, +2) | 1, 1; 245 and 253 |
| A round button (entries 11 to 20, one for each row) | (599, top − 1) | |

What is built is the product's name, " - ", and the turns it takes, with
"t". In a row, the four farmers were 28 pixels apart and two workers or
scientists 30. Hegemon puts people 30 apart, closer when they don't fit in
84 pixels after the first (assumed). With nothing to build, Hegemon leaves the
column empty (assumed).

The parts of a row are x 8 to 97 (the name), 98 to 232 (farmers), 233 to
374 (workers), 375 to 508 (scientists), 509 to 598 (what is built) and 599
to 617 (the round button).

## Clicking

- The name opens the colony's screen. Its Return, and Esc, go back to the
  Colonies screen.
- What is built opens the colony's build list, over its landscape. OK or
  Cancel goes back to the Colonies screen.
- The round button buys what the colony builds, as Buy does in the colony
  screen. It asks first, or says why it can't, as in "You cannot afford
  this increase in production. You are short by 742 BC."
- Pressing a person picks it up, with the people to its right in its job.
  Hegemon puts them in the job they are let go over, in the same row, as
  in the colony screen (assumed). The original's drop wasn't measured.
- A sort button sorts the rows. Hegemon sorts names alphabetically and
  numbers from the largest, keeping the colonies' order when they are equal
  (assumed). Population counts the people, Food what is left over, Industry
  the production, Science the research, Producing the product's name, and
  BC the income.
- The arrows move the rows by one (assumed).

## The colony shown

The panels at the bottom are about one colony. Hegemon shows the colony last
under the pointer, the top one at first (assumed).

At the bottom left, in font 1, spacing 1, colours 245 and 253, from x 13, a
line every 11 pixels from y 354. The lines are ESTRINGS.LBX message 74, one
line each:

| Line | Example |
|---|---|
| Size and climate | Medium Terran |
| Gravity: "Low G", "Normal G" or "Heavy G" with "ravity" | Normal Gravity |
| Minerals | Mineral Abundant |
| People and the most the planet holds | Population (8/12) |
| People on their way | Enroute (0) |
| Growth toward the next person, a turn's growth, and a number not yet understood | 73k +73k (0) |
| Production put in, the cost and production a turn | 0/200 +6 |

Hegemon writes the last number of the growth line as 0.

In the middle are the colony screen's rows of icons, in COLONY2.LBX's
smaller icons from entry 27 (the same order as entries 0 to 15). They
start at x 106, 20 pixels apart. The rows are food at y 349, production
at 367, research at 385 and money at 403. The money row was "3, a gap, 5",
as in the colony screen. How they squeeze together is assumed, within 250
pixels.

## The small map

Every star is at (380 + x × 128 / width, 349 + y × 91 / height) of the
galaxy, rounded down. This was measured for the 36 stars of a 759 × 600
galaxy.

- A star is a cross: colour 135 in the middle and 132 around it.
- A black hole is a ring: 124 beside it, 122 diagonally and 120 in the
  middle.
- The shown colony's star is a cross in 239 and 238 (red).

The star's name is in font 1, spacing 1, centred on x 444 at y 431, with
the same shadow as the colony names.

## The totals

At the bottom right are the empire's totals, in font 2, spacing 2, colours
245 and 253. The words are at x 520 and the numbers end at x 627, a line
every 13 pixels from y 354. They are ESTRINGS.LBX messages 118, 105, 114,
103, 102 and 117:

| Word | Example |
|---|---|
| Reserve: | 58 |
| Income: | +5 |
| Population: | 8 |
| Freighters: | 0 |
| Food: | +0 |
| Research: | 10 |

## Hegemon's own

- The arrow keys or the D-pad move the pointer over the rows' parts and
  the buttons below them, and Enter, Space or A click there.
- Page Up and Page Down, the shoulder buttons and the mouse wheel move the
  rows.
- Esc or B goes back to the map.

## Not measured yet

- Sorting, scrolling, and what the round buttons' pictures are for while
  not held.
- Moving people in this screen (the original took both workers on a press,
  but the drop wasn't caught).
- The number in brackets after the growth.
- Which colony the panels show with more than one colony.
- What the building column shows while people are held. The original
  showed "Colony Base" in a larger font.
