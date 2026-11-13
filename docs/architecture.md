# How Hegemon is built

## A one-to-one copy first

Without add-ons, Hegemon plays exactly like version 1.31 of the 1996 original:
the same rules, screens, computer players, numbers, random galaxies and
savegames. Anything new, such as gamepad controls or scaling to modern
screens, is added on top without changing how the game plays or looks.

When the original's behaviour is unclear, we observe the original program and
write down what it does (see the clean-room rules in `CONTRIBUTING.md`).
Behaviour that has been pinned down gets a test.

## Measuring the original

The original runs only as a measuring instrument, in DOSBox with its
debugger, never as part of Hegemon. A savegame is made there, a turn is
played, and the result is kept as a capture. Hegemon loads the same savegame,
plays the same turn and must come to the same result. Screens are compared
with captures of the original pixel for pixel. Captures stay on the
developer's machine, like the game files; only the facts learned from them
go into `docs/rules/` and the tests.

## Game content is data

The engine knows the mechanisms (production, research, ship design, movement,
space and ground combat, diplomacy, espionage). The content they work on
comes from the original's own files: races, buildings, technologies, ship
parts, leaders, monsters, events, texts and pictures.

## Packs and add-ons

- On first start, Hegemon imports the player's own original files into the
  **base pack** in its library. The base pack is the classic rule set.
- **Add-ons** are packs that are loaded on top of the base pack. They can add
  things or change values, but never change files of the base pack. A
  community patch can be offered this way, never as the default.
- A savegame records which add-ons were active.
- Add-ons hold data only, no program code, so that a downloaded add-on is
  safe to use. Every file in an add-on is checked like any other outside file
  (see `SECURITY.md`).

## Screen and input

The original draws a 640×480 picture with 256 colours. The core draws that
same picture; the app scales it sharply to any window. Mouse, keyboard,
gamepad and touch are equal ways in: the gamepad moves a cursor that snaps to
buttons, planets, ships and list rows, so every screen of the original can be
played without a mouse.
