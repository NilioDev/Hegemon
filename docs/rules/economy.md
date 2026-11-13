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
feudal_science` and `democracy_science` × 5 %.

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

## Still to do

Everything that isn't at a home colony at the start: other buildings,
gravity, morale, the tax rate, research projects, freighters, leaders,
growth, and how computer players arrange their colonies.
