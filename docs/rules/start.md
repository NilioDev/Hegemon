# How a game starts

What each player has on the first turn of a new game with average
technology, as the original with fan patch 1.51 makes it. Measured on 26
new games (each stock race twice as the human player, with four computer
players each), read from the game's own Lua interface after the home star
was named: `get_colonies`, `get_pop_info_of_colony`,
`get_buildings_of_colony`, `get_ships` and the side panel on the screen.

## Each player

- Money: `starting_condition starting_money` (50 BC).
- One colony, on the home world (see [galaxy.md](galaxy.md)), with
  `starting_condition starting_population` people (8).
- Buildings: a star base, marine barracks and a capitol. A Unification
  has no capitol (all Klackon colonies in the measured games).
- Ships at the home star: two Scouts and a Colony Ship, unless
  `no_starting_ships` is 1; an Outpost Ship as well when
  `starting_outpost_ship` is 1.

## Who does what

The home colony's people are put to work like this:

1. As few farmers as grow the food everyone eats (see
   [economy.md](economy.md)). A Lithovore race has none.
2. Of the rest (R people), scientists until they make at least 3/5 × R ×
   `research_scientist_bonus base_scientist` research (with the race's
   science pick, an ancient artifacts world and the government's research
   bonus or penalty), but at least one and never all of them.
3. The others are workers.

This fits all thirteen stock races as the human player:

| Race | Farmers | Workers | Scientists |
| --- | --- | --- | --- |
| Alkari | 4 | 2 | 2 |
| Bulrathi, Darloks, Elerians, Gnolams, Mrrshan | 4 | 1 | 3 |
| Humans, Psilons | 4 | 2 | 2 |
| Klackons, Meklars | 2 | 2 | 4 |
| Sakkra | 3 | 1 | 4 |
| Silicoids | 0 | 3 | 5 |
| Trilarians | 3 | 2 | 3 |

Step 2 is a fit to these thirteen; races with other picks have not been
measured. Computer players start differently (often one more farmer); their
colonies are arranged by the computer players' own rules, which are still
to do.

## Still to do

Pre-warp, post-warp and advanced technology starts (more buildings, ships
and known techs), the techs each player knows, leaders, the starting
marines, and the computer players' job arrangement.
