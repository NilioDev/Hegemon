# Ship designs

What a ship design holds, what it costs and how fast it is in combat, and
the six designs each player starts with. Measured on version 1.51 of the fan
patch at average technology, from 252 designs of the human player in saved
games and from the build list and design screens.

## A design

- A hull size (frigate, destroyer, cruiser, battleship, titan, doom star),
  a shield, drive, computer and armor (rows of the fan patch's tables), the
  special systems (bit n for special n of the special table), up to eight
  weapon mounts and a picture.
- A weapon mount: the weapon table's number, how many, the firing arc
  (1 forward, 2 forward extended, 4 backward, 8 backward extended, 16 all
  round, the last also for missiles and bombs), the mods (bit n for mod
  number n of the weapon_mod table: 2 heavy mount, 4 point defense) and the
  shots of a missile rack or bomb bay.
- The original keeps the player's designs in the saved game as 99-byte
  records: the name (16 bytes), size, a kind byte (1 for a colony ship),
  shield, drive, speed, computer, armor, 5 bytes of special bits, 8 mounts
  of 8 bytes (weapon as 2 bytes, count, working count, arc, mods as 2
  bytes, shots), picture, owner, cost (2 bytes), combat speed and 2 more
  bytes.

## Cost

The sum of, rounded down at the end (fits all 252 designs):

- The hull's cost (hull table) and the computer's for the hull size
  (computer table); the shield's and the drive's for the hull size; the
  armor's cost% of the hull's cost.
- Each weapon: its cost (weapon table; missiles take theirs from
  missile_space_and_cost by their shots: 2, 5, 10, 15 or 20), less a
  quarter a level of miniaturization; then for each mod its cost% of that,
  rounded down (heavy mount doubles it, point defense halves it); then for
  a beam an extended arc adds a quarter and an all-round arc a half,
  rounded down. A nuclear bomb at the start costs 2.25.
- Each special system's cost for the hull size (special table), less a
  quarter a level (not for battle pods).

Miniaturization: how many fields after the part's own field in its
category the player knows. At average technology only the nuclear bomb has
a level (cold fusion follows nuclear fission). Assumed: each further level
takes a quarter of what is left, to at most five levels.

## Room and combat speed

- The room of a hull is the hull table's size. Weapons take their size
  (missiles from missile_space_and_cost) the same way as their cost, but a
  miniaturized size is rounded to the nearest whole (a nuclear bomb takes
  4 of 5); special systems their size for the hull size, extended fuel
  tanks, reinforced hull, heavy armor, battle pods, troop pods and
  augmented engines without miniaturization; the shield its space for the
  hull size.
- Combat speed: the drive's slowest speed for the hull size plus the room
  left over as a share of the hull's room times the difference to its
  fastest, rounded up, never less than the slowest (fits all 252 designs;
  the two starting scouts have 12 rather than 10, as the fan patch's notes
  say).

## The starting designs

Each player has six: the Scout and five made by the auto-designer, one
for each hull size from frigate to titan.

- The Scout: a frigate with one forward beam (the best the player knows),
  extended fuel tanks and the best computer, drive and armor, named
  "Scout" (KENTEXT1.LBX entry 24) and drawn with a random one of the eight
  frigate pictures.
- The others come from the ai_ship_design tables: each column in turn
  gets the column's percent of the hull's room (rounded down) plus what the
  column before left over; a column of -1 is skipped and passes nothing on.
  Missiles, heavy-mount beams, beams, point-defense beams and bombs then fit
  as many of the best known weapon of that kind as the room allows
  ("best": the last in the weapon table that may carry the mod). Special
  columns take the first known special of their list that fits. With
  nothing to put in a column its room goes on to the next. This gives
  every measured design of the standard_beam_missile_x2,
  hybrid_beam_missile_x5 and standard_missile_x10 tables.
- Which table: measured from the missiles the designs carry, x2, x5 and
  x10 racks in about 60, 32 and 8 designs of a hundred; the tables that
  need fighters, special weapons or devices were never seen at the start.
- Beam groups have a forward-extended arc about 10 times in a hundred for
  each hull size above the frigate (none on frigates, about a third on
  titans); the rest forward. Point defense on titans was once seen all
  round.
- The name: one of the race's eight names for the hull size (SHIPNAME.LBX
  entry 0, 16-byte records, 48 a race), at random; the picture is SHIPS.LBX
  entry of the same number.

## Added in Hegemon

The race's design theme and the special weapons of the tables are not
filled in yet, nor are torpedoes known at the start.
