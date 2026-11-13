# Hegemon

**Play the 1996 space strategy classic *Master of Orion II: Battle at
Antares* on today's computers, phones and tablets, without an emulator.**

Hegemon is a free, open-source, from-scratch engine for the original game.
Bring your own copy (version 1.31, DOS or Windows 95): Hegemon reads its files
and plays the game exactly as the original does, with the same rules,
screens, computer players and savegames, scaled sharply to any screen and
with gamepad, mouse, keyboard and touch controls.

> [!NOTE]
> **Status: just started.** Hegemon finds your game files and shows the
> picture of the main menu from them, in the original's colours. There is
> nothing to play yet.

Hegemon is not affiliated with or endorsed by the makers of the original
game. It contains none of the original game's files: you need your own copy.

## Planned

- 🎯 A one-to-one copy of version 1.31: same rules, same look, same computer
  players, same galaxies.
- 🖥️ Windows, macOS, Linux and Android from one codebase.
- 🎮 Full gamepad support: a cursor that snaps to buttons, planets, ships and
  lists, the shoulder buttons to switch between screens and colonies, and
  every button can be changed.
- 💾 Your old savegames keep working.

## Try it

There are no releases yet. Every build on `main` makes test packages: open
the [latest CI run on main](https://github.com/NilioDev/Hegemon/actions/workflows/ci.yml?query=branch%3Amain)
and download the one for your system at the bottom of the page.

## Add your game

Copy the folder of your installed game (the one with the `.LBX` files) into a
folder named `gamedata` next to the program, or start it with the folder:
`./Hegemon path/to/game`. Hegemon then shows the main menu's picture from
your files. Without game files it says where to put them.

## Controls

| Action | Keyboard | Gamepad |
| --- | --- | --- |
| Close the window | Esc | B |

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).
