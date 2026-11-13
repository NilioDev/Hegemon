# Making the galaxy

After the banner (see [banner.md](banner.md)) the original makes a new
galaxy. Fan patch 1.51 does this with tables in `150\docs\config.json` (see
[../formats/parameters.md](../formats/parameters.md)), which the player's
settings files may change. Hegemon makes its galaxies from the same tables,
so a galaxy follows the same chances as one the original makes. It can't
make the very same galaxy from the same seed: that would need the
original's program code, which Hegemon doesn't use (see CONTRIBUTING.md).

What the tables don't say was measured: the fan patch can run a Lua script
after it makes a galaxy (`newgame_postprocessor_script` in `USER.CFG`). A
script that prints `get_game()`, `get_stars()`, `get_planets()` and
`get_players()` to `ORION2.LOG` showed 40 galaxies of the default settings
(medium, average age, five players) and others of every size, age and
number of players. The numbers below are from those galaxies; "measured"
marks what no table gives.

## The map

`map_generation` gives the map's `width` and `height`
for small, medium, large and huge; `stars_in_galaxy` the number of stars.
Medium is 759 × 600 with 36 stars. A cluster galaxy has the map of a large
one and the stars of a huge one, closer together (measured).

All of the following was measured; the tables say nothing about it.

| Size | Margins left, top, right, bottom | No two stars closer than, across and down | Home stars apart |
|---|---|---|---|
| Small | 24, 23, 18, 26 | 57 and 50 | 129 |
| Medium | 31, 31, 38, 38 | 60 and 74 | 290 |
| Large | 42, 42, 43, 47 | 75 and 71 | 430 |
| Huge | 63, 66, 54, 64 | 90 and 88 | 651 |
| Cluster | 31, 33, 28, 39 | 60 and 74 | 430 |

- **Stars** lie inside the margins. No two are closer than the table's
  numbers across and, at the same time, down; a few pairs in the original
  come a little closer. The stars fill the map more evenly than stars
  dropped at random: Hegemon picks two free places at random for each star
  and takes the one farther from the other stars, which gives pairs of
  stars about as often at each distance as the original.
- **Home stars** are at least the table's distance apart; the nearest
  pairs in the original come to just that. When there is no such choice
  (many players in a small galaxy), Hegemon lets them come a tenth closer at
  a time.
- **Orion** is star 0 and lies among the home stars: it is the star nearest
  to the middle of the home stars, or one of the next nearest (in percent,
  the nearest 12, then 17, 17, 15, 12, 7, 7, 5, 2 and 2 for the tenth).

## Stars

The colour of a star other than Orion and the home stars is picked with
the weights in `star_class_chance` for the galaxy's age: blue, white,
yellow, orange, red, brown or black hole. Average age gives about 10 % blue,
16 % white, 16 % yellow, 17 % orange, 37 % red, 1 % brown and 2 % black
holes; measured black holes are fewer than the table's 4 % because the
fan patch turns black holes in nebulae into ordinary stars.

Measured, home stars are 41 % red, 22 % orange, 18 % yellow, 15 % white,
3 % brown and 1 % blue, never black holes. Orion is blue, white, yellow,
orange or red, all about as likely.

## Satellites

A star has five orbits; each holds nothing, an asteroid belt, a gas giant
or a planet. How many are taken: a roll from 0 to 9 picks the row of
`class_to_num_satellites`, the star's colour the column. By default a red
star has 0 to 4 (10, 30, 30, 20 and 10 %), an orange one 2 to 5; black holes
have none. The measured galaxies match the table.

Which orbits: one at a time among the free ones, with the weights in
`satellite_orbit_chance` for the galaxy's age (all 20 at average age, so
every orbit is as likely).

What is in an orbit: a roll from 0 to 9 picks the row of
`orbit_to_satellite_type`, the orbit its column: 1 an asteroid belt, 2 a
gas giant, 3 a planet. The table's 4 (in the first orbit) gives an asteroid
belt or a planet, about evenly (measured).

Every satellite gets its minerals from `class_to_mineral`: a roll from 0
to 9 picks the row, the star's colour the column (0 ultra poor to 4 ultra
rich). A planet also gets:

- **size**: `planet_size_chance` counts up to 10; a roll from 1 to 10 takes
  the first size whose number it doesn't pass (by default 10 % tiny, 20 %
  small, 40 % medium, 20 % large, 10 % huge);
- **gravity**: `planet_size_to_gravity`, its minerals the row and its size
  the column (0 low, 1 normal, 2 heavy);
- **climate**: `orbit_to_climate_group` gives the star colour's group for
  the orbit (bombarded, hostile, life supporting, lifeless), and the
  group's column of `climate_chance_young_avg_gal` (young and average
  galaxies) or `climate_chance_old_gal` gives the weights of the ten
  climates, toxic to gaia.

## Orion

Orion's planet is huge, ultra rich, of normal gravity and gaia, in the
third, fourth or fifth orbit (measured: 20 %, 50 % and 30 %). Each of
Orion's other orbits holds an asteroid belt half of the time and nothing
else (measured).

## Home systems

Each player's home star gets three, four or five satellites (measured:
50 %, 34 % and 16 %), made as above until at least three of them are
planets (the fan patch's `/planets` switch, whose default is 3). One of
those planets, any of them, becomes the home world:

- medium, or large for a race with Large Home World;
- abundant, rich for Rich Home World, poor for Poor Home World;
- normal gravity, low for Low-G World, heavy for High-G World;
- terran, or ocean for an aquatic race.

No other planet of a home system is terran (measured): Hegemon rolls such
a planet's climate again.

## Names

A home star has its race's name: entry 0 of `STARNAME.LBX` (a text table,
see [../formats/text.md](../formats/text.md)) holds one per race, in the
race selection's order; the Alkari's is Altair. Orion is called Orion.

The other stars take theirs from entry 1, 829 names. Measured, star i has
name i of that list about one time in seven, and otherwise any name: the
list is mixed by swapping two names picked at random, as many times as
the list has names, and star i takes name i.

## Special finds

A star other than Orion and the home stars, with at least one satellite,
may get a special find. `star_special_chance` (64) says "a roll from 0 to
99 above the value gives a find", 36 in 100; measured, 41 in 100 such
systems get one in the original, and Hegemon uses that (41/36 of the
setting's chance). `planet_special_chance` weighs which find (its
`no_special` and `monster` weights are not used here). A find that has no
place in the system is rolled again, up to 20 times:

- **wormhole**: the other end is a star of the same kind with no find and
  no wormhole, far enough away (measured: 125, 120, 150, 210 and 190 for
  small, medium, large, huge and cluster). Neither end has another find;
- **space debris, pirate cache, hero**: the system has it, no planet;
- **gold or gem deposits**: a planet of the system, picked at random;
- **natives, splinter colony, ancient artifacts**: a planet whose climate is
  desert or better. Its climate then is (measured, in percent) desert 16,
  tundra 18, ocean 7, swamp 12, arid 8, terran 25, gaia 13. Natives and
  splinter colonies make a tiny or small planet medium, large or huge.

Measured with the default weights, natives, splinter colonies and ancient
artifacts come out much rarer than their weights (9, 4, 5) say, because
few systems have a planet good enough; wormholes come out a little more
often.

A race with Artifacts Home World has ancient artifacts on its home world.

## Monsters

The Guardian guards Orion. Other monsters guard systems that have a find
and a planet, each such system with a chance of 24 in 100 (measured), at
most 1, 3, 5, 7 and 7 systems for small, medium, large, huge and cluster.
Which monster depends on the find (measured counts):

| Find | Amoeba | Crystal | Dragon | Eel | Hydra |
|---|---|---|---|---|---|
| Space debris | 2 | 1 | 0 | 0 | 5 |
| Pirate cache | 4 | 0 | 4 | 0 | 5 |
| Gold deposits | 3 | 2 | 0 | 17 | 5 |
| Gem deposits | 4 | 0 | 5 | 9 | 3 |
| Natives | 3 | 6 | 19 | 0 | 13 |
| Splinter colony | 6 | 5 | 0 | 7 | 4 |
| Hero | 7 | 1 | 5 | 0 | 4 |
| Ancient artifacts | 3 | 7 | 5 | 0 | 9 |

A guarded system's planets are better (measured: three in four rich or
ultra rich, nearly two in three large or huge, one in three terran or
gaia; gravity stays as it was). Hegemon makes each planet rich or ultra
rich with a chance of 65 in 100, large or huge with 50 and terran or gaia
with 30.

## Still to do

Nebulae (and black holes in them), the Antarans, and the fan patch's map
switches (`mapgen` table).
