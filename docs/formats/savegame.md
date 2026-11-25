# Saved games (SAVE*.GAM)

The original keeps its saved games as `SAVE1.GAM` to `SAVE9.GAM` and the
autosave made at the start of each turn as `SAVE10.GAM`. Measured on fan
patch 1.51: 26 saved games paired with the Lua interface's dump of the same
moment (stars, planets, players, colonies, ships), and games played on from
one save in the original with one change at a time (research chosen, build
lists filled, a turn ended) to see which bytes change.

Every saved game of 1.51 is 223,120 bytes. Numbers are little-endian;
offsets are from the start of the file unless a record is named. A text is
padded with zero bytes. Hegemon refuses a file of another size or version
and checks every number it uses against what it may be.

## The file

| Offset | Size | What |
|---|---|---|
| 0 | 4 | Version, 0xE0 |
| 4 | 37 | The save's name, such as "(Auto Save)" |
| 41 | 2 | Stardate in tenths of a year (35000 is 3500.0) |
| 603 | 250 × 361 | Colonies |
| 90855 | 2 | Number of planets |
| 90857 | 360 × 17 | Planets |
| 96977 | 2 | Number of stars |
| 96979 | × 113 | Stars |
| 109071 | 8 × 3753 | Players |
| 139094 | 2 | Number of ships |
| 139096 | × 129 | Ships |
| 203753 | 2 + 2 | Map width and height (759 × 600 for a medium galaxy); Hegemon finds the map size by the width in `map_generation` |
| 203759 | 4 × 5 | Nebulae: x and y (2 bytes each), picture |
| 203779 | 1 | Number of nebulae |

## A star (113 bytes)

| Offset | Size | What |
|---|---|---|
| 0 | 15 | Name |
| 15, 17 | 2, 2 | x and y on the map |
| 19 | 1 | How big the galaxy map draws it, 0 to 2 |
| 22 | 1 | Colour, blue to brown, 6 a black hole |
| 40 | 1 | Special, in the order of `planet_special_chance` (11 Orion) |
| 41 | 1 | The star at the other end of its wormhole, −1 for none |
| 51 | 1 | Which players have been there, bit *n* player *n*. Every player has every black hole; the original names none of them on the map |
| 74 | 5 × 2 | The planet in each orbit, −1 for none |
| 111 | 1 | In a nebula |

## A planet (17 bytes)

| Offset | Size | What |
|---|---|---|
| 0 | 2 | Its colony, −1 for none |
| 2 | 1 | Star |
| 3 | 1 | Orbit, 0 nearest the star |
| 4 | 1 | 1 asteroid belt, 2 gas giant, 3 planet |
| 5 | 1 | Size, tiny to huge |
| 6 | 1 | Gravity: low, normal, heavy |
| 8 | 1 | Climate, toxic to gaia |
| 9 | 1 | Which of the climate's landscapes (0 to 2) |
| 10 | 1 | Minerals, ultra poor to ultra rich |
| 11 | 1 | Food base |
| 15 | 1 | Special |

## A player (3753 bytes)

| Offset | Size | What |
|---|---|---|
| 0 | 20 | Ruler's name; empty for a player who isn't in the game |
| 20 | 15 | Race's name |
| 36 | 1 | Race, in the race selection's order |
| 37 | 1 | Banner |
| 40 | 2 | Home planet |
| 49 | 4 | BC in the treasury |
| 195 | 1 a field | Each field by its `tech_field` id: 3 known, 2 can be researched, 1 not yet |
| 278 | 1 a tech | Each application by its `tech` id: 3 known |
| 490 | 2 | Research points put into the field being researched |
| 800 | 1 | The field being researched, 0 for none |
| 801 | 1 | The application chosen in it, by its `tech` id |
| 805 | 6 × 99 | The designs it builds (see [ship design](../rules/ship-design.md)) |
| 2206 | 31 | The race's picks, as RACESTUF.LBX keeps them (see [races](races.md)); the same 31 bytes follow again at 2237 |

The first player is the human. Players 9 to 14 own the monsters: the
Guardian, the Amoeba, the Crystal, the Dragon, the Eel and the Hydra.

## A ship (129 bytes)

The first 99 bytes are its design: name (16), hull (16), type (17: 0 a
warship, 1 colony ship, 2 transport, 4 outpost ship), shield (18), drive
(19), speed (20), computer (21), armour (22), specials as 40 bits (23),
eight weapons of 8 bytes (28: weapon number (2), count, working count, arc,
mods (2), shots), picture (92), player (93), cost (94, 2 bytes) and combat
speed (96). Then:

| Offset | Size | What |
|---|---|---|
| 99 | 1 | Owner |
| 100 | 1 | 0 orbiting a star, 1 on its way, 6 in a colony's build list; 5 (owner 8) not understood yet |
| 101 | 2 | The star it orbits, or 500 + the star it travels to |
| 103, 105 | 2, 2 | x and y on the map |
| 108 | 1 | Parsecs a turn on its way |
| 109 | 1 | Turns to go |
| 116 | 2 | Its leader, −1 for none |

A ship that a colony builds is in the list of ships from the moment it is
put in the build list, as its design.

## A colony (361 bytes)

A slot whose owner is 0xFF holds no colony.

| Offset | Size | What |
|---|---|---|
| 2 | 1 | Owner |
| 4 | 2 | Planet |
| 12 | 1 | Number of people |
| 14 | 4 each | People: bits 0–3 race, 4–6 loyalty, 7–8 job (0 farmer, 1 worker, 2 scientist) |
| 202 | 2 | Thousands of people grown towards the next one |
| 279 | 7 × 2 | The build list, first at the front: a building by its `building` id (the colony base is building 11), −1 nothing, −2 trade goods, −3 housing, −7 a spy, −15 freighters, −(100 + *n*) the ship number *n* |
| 312 + id | 1 a building | Has the building of `building` id *id* |

## Not read yet

How far the first of a build list is, spies, freighters, leaders,
diplomacy, the other players' sight of the stars, ships with status 5, and
the colony screen's landscape layout (not in the file: Hegemon picks one
when it loads a game).
