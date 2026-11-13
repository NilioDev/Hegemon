# Hegemon

**Play the 1996 space strategy classic *Master of Orion II: Battle at
Antares* on today's computers, phones and tablets, without an emulator.**

Hegemon is a free, open-source, from-scratch engine for the original game.
Bring your own copy: Hegemon reads its files and plays the game exactly as
the original with the community fan patch 1.51 from
[moo2mod.com](https://moo2mod.com/) does, with the same rules, screens,
computer players and savegames, scaled sharply to any screen and with
gamepad, mouse, keyboard and touch controls.

> [!NOTE]
> **Status: just started.** Hegemon shows the original's main menu from your
> files, with its opening animation, scrolling credits, version text and
> mouse pointer. A click or a key skips the opening. New Game opens the New
> Game screen, where you can pick the difficulty, galaxy, players and the
> other settings, and Accept leads to choosing your race, entering your
> ruler's name and choosing your banner colour. Making the galaxy comes
> next; until then Esc there goes back to the main menu. Continue and Load Game are
> only usable when you have savegames, as in the original, and the buttons
> light up under the mouse or the gamepad. Quit Game closes Hegemon; Continue, Load Game,
> Multi Player and Hall of Fame don't lead anywhere yet.

Hegemon is not affiliated with or endorsed by the makers of the original
game. It contains none of the original game's files: you need your own copy.

## Planned

- 🎯 A one-to-one copy of the game with fan patch 1.51: same rules, same
  look, same computer players, same galaxies.
- 🧩 The fan patch's own settings, scripts and mods are read from your game
  folder, so changes made there work in Hegemon too.
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
`./Hegemon path/to/game`. Install the fan patch 1.51 from
[moo2mod.com](https://moo2mod.com/) in that folder: Hegemon plays the game as
the patched version does and reads its settings and mods. Hegemon then shows
the main menu from your files. Without game files or the fan patch it says
what is missing.

## Controls

| Action | Mouse | Keyboard | Gamepad |
| --- | --- | --- | --- |
| Choose a button in the main menu | Point and click | Up / Down, then Enter, or the button's first letter | D-pad or left stick, then A |
| Change a setting on the New Game screen | Click its picture or switch | Enter clicks where the pointer is | D-pad or left stick to move, then A |
| Leave the New Game screen | Cancel or Accept | Esc | B |
| Choose a race | Point and click | Enter clicks where the pointer is, Esc goes back | D-pad or left stick, then A; B goes back |
| Enter your ruler's name | Accept | Type, Backspace, Enter; Esc goes back | A takes the offered name; B goes back |
| Choose your banner colour | Click a banner | Enter clicks where the pointer is, Esc goes back | D-pad, then A; B goes back |
| Close Hegemon | Quit Game | | |
| Close the message when there are no game files | | Esc | B |

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).
