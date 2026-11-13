# Races

The thirteen stock races and the texts about racial picks are in
`RACESTUF.LBX`. Texts are lists of words, each ended by a zero byte.

| Entry | What |
| --- | --- |
| 0 | English pick names and levels (the custom race screen): "Population", "-50% Growth", ... |
| 1 to 4 | The same in German, French, Spanish and Italian |
| 5 | The same as 0 |
| 6 | 57 signed bytes, probably the picks' costs (not used yet) |
| 7 | The stock races' picks |
| 8 | English labels for the race selection: "government", "Population Growth:", ... |
| 9 to 12 | The same in German, French, Spanish and Italian |
| 13 | The same as 8 |

## The stock races (entry 7)

A table like the text tables (see [text.md](text.md)): a count of 13 and a
record size of 31, then one record a race in the order Alkari, Bulrathi,
Darloks, Elerians, Gnolams, Humans, Klackons, Meklars, Mrrshan, Psilons,
Sakkra, Silicoids and Trilarians. Each byte is a pick:

| Byte | Pick | Values |
| --- | --- | --- |
| 0 | Government | 0 Feudal, 2 Dictatorship, 4 Democracy, 6 Unification |
| 1 to 9 | Growth, farming, industry, science, money, ship defense, ship attack, ground combat, spying | 0 none, 1 the bad pick, 2 and 3 the good ones |
| 10 to 30 | Low-G, high-G, aquatic, subterranean, large home world, home world richness, artifacts world, cybernetic, lithovore, repulsive, charismatic, uncreative, creative, tolerant, fantastic traders, telepathic, lucky, omniscient, stealthy ships, trans dimensional, warlord | 0 or 1; the richness is 1 for rich (2 for poor is assumed) |

Hegemon refuses a table that isn't 13 records of 31 bytes.

The levels are worth, from 1 to 3 (the fan patch's defaults): growth −50,
+50 and +100 %; food −½, +1 and +2; industry and science −1, +1 and +2;
money −½, +½ and +1 BC; ship defense −20, +25 and +50; ship attack −20, +20
and +50; ground combat and spying −10, +10 and +20.

## The race selection's text

Each pick that isn't 0 gives one part, in the order of the bytes: the label
from entry 8 (at the pick's number; "Poor Home World" is the 32nd word),
with ": +50" and the like for the picks with levels. Label 3 lacks its
colon in the file ("Industrial Production"); the original shows one. The
government comes last, by its name from entry 0, where the four
governments follow the word "Governments".

How values that aren't whole (−½) are shown has not been measured.

## The fan patch's settings

`stock_race` changes the stock races (see
[config.md](config.md)), in two forms:

```
stock_race Humans government = democracy;
stock_race Sakkra growth = 3;
stock_race Bulrathi = feudal g2 f0 i0 s0 m0 sd3 sa0 gc0 spy0 0 0 0 0 1 -1 0 ...;
```

The second form gives all 31 picks in table order. Letters before a number
are left out, and −1 for the richness means poor. Hegemon refuses a race,
pick or value it doesn't know. The `race_pick` settings for costs and
values are not read yet.

## Ruler names

Entry 0 of `RACENAME.LBX` is a table of texts (see [text.md](text.md)) of
104 records of 20 bytes: eight names for each of the thirteen races, in the
race selection's order, so the Alkari's are records 0 to 7 ("Ariel",
"Heggira", ...). The longest has 14 letters. Hegemon refuses a table with
fewer than 104 records.
