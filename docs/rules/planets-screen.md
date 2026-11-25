# The Planets screen

Measured on fan patch 1.51 under DOSBox, in the Alkari's game at stardate
3500.1. Only Altair was explored and the colony ship was at home. Hegemon's
screen was compared pixel for pixel with the original's. What one system
can't show is marked as assumed.

## Opening it

The PLANETS button under the galaxy map opens the screen. It is the
button's plate at about x 86 to 161, y 436 to 473. Hegemon also opens it
with P, or the gamepad's Y. Return, or Esc, goes back to the map.

## The picture

The screen is PLNTSUM.LBX entry 0, with its own palette for all 256
colours. The pointers are FONTS.LBX entry 7's, which has the same colours.
Parts of the picture are see-through and black.

| Entry | What | Where | Frames |
|---|---|---|---|
| 1 to 3 | Sort by Climate, Minerals, Size | (441, 200), (501, 200), (567, 200) | the one chosen in its second |
| 4 to 8 | No Enemy Presence, Normal Gravity, Non-Hostile Environment, Mineral Abundance, Planets In Range | x 441, y 266, 289, 312, 335, 358 | on in the second |
| 9 | Send Colony Ship | (454, 386) | no ship, a ship, chosen |
| 10 | Send Outpost Ship | (454, 413) | the same |
| 11, 12 | The arrows above and below the list | (422, 16), (421, 448) | held in the second (assumed) |
| 14 | Return | (454, 440) | held in the second |
| 15 to 22 | A star on the small map with a colony, in its owner's banner colour (red, yellow, green, silver, blue, brown, purple, orange) | | 5, animated |
| 23 | A star without one | | 5 |
| 25 | A black hole | | |
| 26 to 75 | A planet: 26 + 5 × climate + size | | |
| 76 to 83 | The mark of a ship sent to a planet, by banner colour | | |

At first only Planets In Range is on and Climate is chosen. With a colony
ship, Send Colony Ship is chosen. Between the arrows, a grey bar fills the
track (x 423 to 431, y 38 to 444) in columns of colours 150, 149, 149, 148,
148, 147, 147, 146, 146. Hegemon shortens it for a list longer than the
screen (assumed).

## The list

Eight rows, their tops at y 36, 91, 145, 200, 255, 310, 365 and 420. Each
row is about one planet. The text is in colours 64 and 65, centred:

| What | Centre | From the top | Font, spacing |
|---|---|---|---|
| The planet's picture | (61, +21) | | |
| Its name | 61 | +29 | 2, 2 |
| Climate, as "Radiated" | 140 | +12 | 2, 2 |
| "%d Food" (HESTRNGS.LBX 321), what a farmer grows | 141 | +28 | 1, 1 |
| Gravity, as "Normal G" | 217 | +18 | 2, 2 |
| Minerals, as "Poor" | 311 | +12 | 2, 2 |
| "%d prod/worker" (338) | 311 | +28 | 1, 1 |
| Size, as "Medium" | 386 | +12 | 2, 2 |
| "%d max pop" (323) | 386 | +28 | 1, 1 |
| The mark of a ship sent there | (90, +38), its top left | | |

The measured rows: "Altair I: Radiated, 0 Food, Normal G, Poor, 2
prod/worker, Medium, 4 max pop" and "Altair III: Toxic, 0 Food, Normal G,
Abundant, 3 prod/worker, Large, 5 max pop". A medium radiated planet holds 4
people. That is 15 × 25% = 3.75, rounded (Hegemon rounds to the nearest).

The list holds the planets of the explored stars that nobody lives on. Gas
giants and asteroid belts are left out (assumed).

The original names the first planet of Altair "Altair I". In the same
galaxy, Sal's first planet is "Sal Prime". Hegemon still calls every
first planet "Prime" until the rule is known.

## Sorting and filters

Climate sorted radiated before toxic, and Size sorted large before medium.
Hegemon sorts by the climate's, minerals' or size's number, the largest
first, keeping the galaxy's order when they are equal (assumed).

The filters hide planets (Hegemon's reading of their names, assumed):

- No Enemy Presence hides stars with another player's colony or a monster.
- Normal Gravity hides low and heavy gravity.
- Non-Hostile Environment hides toxic, radiated and barren planets.
- Mineral Abundance hides poor and ultra poor planets.
- Planets In Range hides planets more than 4 parsecs from the player's
  colonies.

## Sending a ship

- A click on Send Colony Ship or Send Outpost Ship chooses it, and a second
  click puts it out. Without such a ship the button can't be chosen.
- A click on a planet chooses it. A red square (colour 212, 9 pixels wide)
  goes around its star on the small map. Under the map, "%d turns to %s"
  (HESTRNGS.LBX 106, or 105 for one turn) is shown in font 2, colours 219 and
  222, centred on 533 at y 150. For example, "0 turns to Altair" for a ship
  already there.
- With a ship button chosen, the click also sends the nearest such ship.
  The message is "Colony Ship has a course laid for Altair, ETA 0 turns"
  (317 or 318). Once it is closed, the planet's row shows the mark.
- A ship already sent to a planet asks first: "%s has set a course for %s,
  should I re-route it to %s?" (313). Hegemon asks this for a ship sent from
  this screen (assumed).

Hegemon sends the ship to the star. It doesn't land on the planet by
itself yet, and the planet it was sent to isn't saved.

## The small map

Every star is a dot centred on (443 + x × 180 / width, 17 + y × 116 /
height) of the galaxy, rounded down. This was measured for the 36 stars of
a 759 × 600 galaxy. The colony's star blinks through its five frames;
Hegemon draws the first.

## Hegemon's own

- The arrow keys or the D-pad move the pointer over the rows and buttons,
  and Enter, Space or A click there.
- Page Up and Page Down, the shoulder buttons and the mouse wheel move the
  list.
- Esc or B goes back to the map.

## Not measured yet

- Planet names: when the first planet is "Prime".
- What the filters do, the range, the gravity's production penalty
  ("-%d%% prod", 322) and the empty list ("No planets are in the list",
  335).
- How the colony star blinks, the scroll bar of a long list, and what a
  ship sent from here does when it arrives.
