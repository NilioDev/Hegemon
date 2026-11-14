# What a colony makes

Food, production, research and money of a colony in a turn, as far as
measured on the original with fan patch 1.51 (the home colonies at the start
of a game, see [start.md](start.md)). All numbers come from the fan patch's
settings; the names below are theirs.

## Food

Food is counted in halves, as the settings do.

- A farmer grows `food_farmer_base` of the planet's climate, plus the
  race's farming pick (`race_pick farmingN_value`). An aquatic race farms an
  ocean world like a gaia one (measured on the Trilarians' home world).
- A Unification grows `govt_bonus unification_food` × 5 % more.
- Everyone eats one food (two halves); a cybernetic race half a food; a
  Lithovore race nothing.

The side panel shows what is left over, in whole food.

## Production

- A worker makes `production_worker_base` of the planet's minerals, plus the
  race's industry pick (`race_pick industryN_value`).
- A Unification makes `govt_bonus unification_production` × 5 % more.
- Pollution: of what is made beyond `planet_no_pollution_prod` of the
  planet's size, half (rounded up) is lost. A tolerant race has no
  pollution.
- A cybernetic race's people each need half a production.

## Research

A scientist makes `research_scientist_bonus base_scientist`, plus the race's
science pick and `research_scientist_bonus artifacts_planet` on an ancient
artifacts world. Feudal and Democracy change this by `govt_bonus
feudal_science` and `democracy_science` × 5 %. A colony's research is
rounded to the nearest point (measured: three Elerian scientists under
feudalism make 4.5, shown as 5 RP).

## Money

Money is counted in halves too.

- Taxes: one BC per person, plus the race's money pick (`race_pick
  moneyN_value`, in halves). A Democracy gets `govt_bonus democracy_money`
  × 5 % more taxes.
- Trade goods: with nothing to build, the production left after the
  cybernetic upkeep becomes money at `trade_goods_ratio` percent
  (`fantastic_traders` for such a race).
- Upkeep: each building's `maintenance` from the `building` table.

The side panel shows the sum, a half rounded up (+6.5 BC shows as +7).

## Command points

`command_points starting`, plus `star_base`, `battlestation` or
`star_fortress` for each colony with one, plus `warlord_planet` for each
colony of a warlord race. Each ship uses `ship_command_points_usage` of its
class; scouts, colony ships and outpost ships are frigates. The panel shows
what is left and, in brackets, the total.

## Population

How many of a race's people a planet holds: `pop_max` of its size times
`pop_climate` of its climate in percent (an aquatic race counts an ocean
world as gaia), then `pop_bonus_tolerant` percent more for a tolerant race
and `pop_bonus_subterranean` of the size more for a subterranean one.

A colony grows each turn by the fan patch's formula: the square root of
`growth_formula_factor` × people × (room − people) ÷ room, rounded down,
then `growth_rate_bonus general_modifier` plus the race's growth pick
(`race_pick growthN_value`) in percent more. This fits the game's own
growth a turn at every stock race's home colony at the start (Alkari 73,
Sakkra 206 with room 24, Silicoids 43 with room 15, Trilarians 86 on
their ocean world). Every thousand grown is one more person (measured:
Pop 8,073 k after the first turn). Assumed:
the new person works in industry, and the rounding of the bonuses.

## Still to do

Everything that isn't at a home colony at the start: other buildings,
gravity, morale, the tax rate, freighters, leaders, the other races and
techs that change growth, starving, and how computer players arrange their
colonies.
