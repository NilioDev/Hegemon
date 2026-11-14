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
> ruler's name and choosing your banner colour. Hegemon then makes a new
> galaxy the way the fan patch does and shows it on the galaxy map, where you
> name your home star. Every empire starts with its home colony, ships and
> money as in the original, and the side panel shows the stardate, your
> treasury and income, command points, food and freighters. The Turn button
> (or Enter, or Start on a gamepad) ends the turn: the first time it asks
> which field to research, on the Select New Research screen with your race's
> scientist, where the application you point at is the one your scientists
> learn (Page Up/Page Down or the shoulder buttons choose it too), and each
> turn adds your income and research, with the panel showing about how many
> turns the project takes. When a field is finished your scientist shows the
> new application turning in a beam of light, with its description, before
> you choose the next field. After the first turn your
> home colony is shown with its landscape, buildings and roads, the planets
> of its system, its rows of money, food, production and research, its
> government and its people at work, whom you move between farming, working and research by dragging them, and it grows as in the original.
> Change opens its build list, where you put trade goods, housing, ships and
> spies in the colony's list of up to seven things to build and see what each
> costs and how many turns it takes, with the original's picture and description of each. Each turn the colony builds the first of
> the list: buildings, colony, outpost and transport ships, your six
> warship designs (the Scout and five designed for you as in the original,
> with their parts listed in the build list), freighters, spies and colony
> bases, while trade goods earn money and housing speeds up growth,
> and the colony screen shows what is being built and how far it is. Buy
> finishes it at once for BC, at the original's price, and a second Buy in
> the same turn takes the purchase back, as the fan patch allows. Esc on
> the galaxy map goes back to the main
> menu. Continue and Load Game are
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
| Name your home star | Accept | Type, Backspace, Enter | A takes the offered name |
| Choose the next research | Click a field's application | Arrow keys choose the field, Page Up / Page Down the application, then Enter | D-pad chooses the field, LB / RB the application, then A |
| Close the research done screen | Click | Any key | Any button |
| Leave the colony screen | Return | Esc or Enter | B |
| Move people to another job | Drag a person (and those to its right) to another row | Up / Down choose a row, Space picks up its last person, Up / Down choose the new row, Space puts it down | D-pad up / down, then A, the same way |
| Open the colony's build list | Change | C | X |
| Buy what the colony builds | Buy | B | Y |
| Answer a question | Yes or No | Enter or Y, Esc or N | A or B |
| Close a message | Close | Enter or Esc | A or B |
| Add to the build list, or delete a line of it | Click it | Arrow keys, then Space | D-pad, then A |
| Leave the build list | OK keeps it, Cancel doesn't | Enter keeps it, Esc doesn't | Start keeps it, B doesn't |
| Leave the galaxy map (for now) | | Esc | B |
| Close Hegemon | Quit Game | | |
| Close the message when there are no game files | | Esc | B |

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).
