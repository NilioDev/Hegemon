# The banner colour

Enter or Accept on the ruler's name (see [ruler-name.md](ruler-name.md))
opens the Select Banner Color screen. Measured on the original's DOS
program with fan patch 1.51 under DOSBox; the numbers are entries of the
fan patch's `150\lbx\RACESEL.LBX`. Hegemon's drawing matches the capture
for the Alkari pixel for pixel, apart from the pointer.

## What is drawn

1. Entry 0 of `RACEOPT.LBX` with its colours 0 to 127.
2. The panel with the title, entry 32, at (55, 67). It carries colours 128
   to 255, in place of the portrait's.
3. The race's eight banners: entries 34 + 8 × race + colour, with the race
   in the race selection's order (Alkari 0 to Trilarians 12). Top row at
   y = 145: red, yellow, green, silver. Bottom row at y = 281: blue, brown,
   purple, orange. The columns are at x = 96, 220, 344 and 467. The banners
   are 83 to 86 pixels wide and 94 or 95 high, and each is drawn from its
   own corner.
4. The pointer: picture 0 of entry 10 of `FONTS.LBX`.

Nothing lights up under the pointer.

## The mouse and the keys

- A banner is taken when the mouse button is let go over its picture's
  rectangle, wherever the button went down. Pressing on a banner and
  letting go elsewhere does nothing.
- Esc goes back to the ruler's name, which offers a new random name.

**Added in Hegemon:** Enter clicks where the pointer is, as on the other
screens. With a gamepad, the D-pad moves the pointer from banner to banner
(left and right, up and down for the other row), A takes the banner and B
goes back.

## Making the galaxy

Taking a banner fades the screen to black in about a sixth of a second.
Then `RACEOPT.LBX` entry 0 comes back at once with messages under way:

- "Generating Universe ..." (text 242 of `HESTRNGS.LBX`, see
  [../formats/text.md](../formats/text.md)), then "Placing home worlds ..."
  (text 161), and maybe more.
- Font 4 of `FONTS.LBX`, two pixels between letters, centred on the
  screen at y = 215, in colours 119, 124 and 123 when fully drawn.
- The pointer turns into picture 6 of entry 10 of `FONTS.LBX`.

Then the screen goes black and the main game screen fades in, asking for the
home star's name.

Hegemon draws the first message, fully drawn, and stays there, because it
can't make the galaxy yet; Esc or the gamepad's B goes back to the main
menu.

## Still to do

- How the messages are drawn. Each brightens over about ten frames, and
  around the letters there is a dark shape in colour 2 that is not a plain
  shadow at fixed offsets. It needs frame-by-frame captures.
- Which messages come in which order, and how long each stays. That
  belongs with making the galaxy.
- Banners for a custom race.
