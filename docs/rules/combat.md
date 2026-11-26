# Space combat

Measured on fan patch 1.51 under DOSBox, from saved games edited to bring
a ship and a monster together, and from the fan patch's own manual
(`150/docs/MANUAL_150.PDF`, "Notes on Beam Weapon Mechanics" and the notes
after it), which gives the formulas the patched game uses. What is neither
is marked as assumed.

## Who fights

A space monster guards its star. At the end of a turn, after the ships
have moved, every monster fights the ships of the players at its star
(measured: a scout sent into the Hydra's system and a Hydra moved to a home
system both fought at once). The original's manual: a fleet with no
warships is destroyed out of hand. A ship that dies at a star it reached
that turn still explored it: the original showed "Scouts arrive at the Ell
system" after the battle (measured).

## The message

Before the battle the galaxy map shows the box of WARNING.LBX 0 at
(154, 144), its lights going back and forth as in the Load Game warning,
with ESTRINGS.LBX 49 "%s attacks %s at the %s system": the monster
(159 to 163 "A Space Amoeba" ... "A Space Hydra", 514 "The Guardian"), the
race and the star. The text is font 4, two pixels between characters,
shades 75, 129 and 130, in lines 17 pixels apart centred on the warning's
line (y 223 and 240 for two lines) and on x 322, all measured. The lines
are wrapped at 230 pixels (assumed: the measured lines need at least 218
and at most 239). Return closes it (measured). An attack on a colony says
ESTRINGS.LBX 48 "%s attacks %s at colony of %s" (seen; monsters don't
leave their systems in Hegemon yet). The original still shows the date of
the turn that ended under the message; Hegemon shows the new one.

## The battle

Hegemon fights the battle the way the combat screen's AUTO does: the
original fought the scout against the Hydra in under a second and the
scout died. The combat screen itself is still to come.

- **The units.** A ship's structure is its hull's `structure_hp`, its
  armour the hull's `armor_hp` raised by the armour's `ships_bonus` percent,
  as the combat screen showed for the Alkari scout (structure 4, armour 4,
  drive 2, computer 1: the frigate's hit points). Its beam attack and
  defense are those of the Fleets screen, its movement its combat speed
  (12 for the scout, measured "Max Speed 12"). A monster comes from the
  fan patch's monster tables: the Hydra's structure 500
  (`monster_hit_points`), no armour, three Plasma Breaths and a speed of 8
  (`monster_drive` fusion on a cruiser hull) are what the combat screen
  showed. The Guardian has no table: Hegemon gives it a titan hull, 10000
  structure and the moleculartronic computer (assumed).
- **Beams.** The manual's classic formula, with a roll from 1 to 100: a
  roll over 95 hits, and so does a beam attack over beam defense of 99 or
  more, both with the most damage; otherwise the shot hits when the roll
  plus attack less defense reaches 40 plus the range penalty (at most 95),
  and does the least damage plus (most - least + 1) × how far it got past
  that, over how far it could have got, at most the most. The range is a
  range for every three squares; the to-hit penalties by range are 0, 0,
  10, 20, 30, 40, 55, 70, 85; a heavy mount halves the range, point
  defence doubles it, and weapons with the double range penalty double the
  penalty. Beams that dissipate keep 100, 100, 90, 80, 70, 60, 50, 40 and
  35 percent of their damage by range (the manual's examples), rounded,
  at least 1; heavy mounts add 50 percent, point defence takes 50. The
  fan patch fights with the weapon table's strategic damage columns.
- **Damage.** Each hit loses the shield's class to the shields (assumed),
  the armour takes what it can hold, the structure the rest; at no
  structure the unit is destroyed. Internal systems are not hit yet.
- **Turns.** Every combat turn each unit, fastest first (assumed), moves up
  to its movement a square a point towards the nearest enemy until it is
  next to it, and fires every beam at it. The battle ends when one side is
  left or after `max_combat_turns` turns. Missiles, torpedoes, bombs,
  fighters and special weapons don't fire yet.
- **The field.** 80 by 52 squares of 20 pixels (assumed: the combat
  screen's grid is 20 pixels and its small map shows about 1600 pixels
  across); the ships start a quarter of the way in on the left, two
  squares apart, the monster three quarters in, as the screen showed.

Ships destroyed leave the game. A monster that survives is whole again
next turn (assumed).
