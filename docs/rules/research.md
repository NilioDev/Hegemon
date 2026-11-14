# Research

How the technology tree is read, what a player knows at the start, how a
turn adds research and how the Select New Research screen looks, as far as
measured on the original with fan patch 1.51.

## The tree

Everything comes from the fan patch's settings, so a new fan patch with
other fields, costs or applications needs no change in Hegemon:

- `tech_field`: each field's `id`, the `next_field_id` after it in its
  category and its `cost` in research points. Field 0 is Starting Tech.
- `tech`: each application's `id` and its `tech_field_id`. A field's
  applications are listed in the order of the settings' rows (measured: the
  screen lists Advanced Engineering's as Anti-Missile Rockets, Reinforced
  Hull, Fighter Bays).
- `starting_techfield`: the first field of each category, in the order
  Biology, Power, Physics, Construction, Force Fields, Chemistry, Computers,
  Sociology, Xenon. A category is the chain from there along
  `next_field_id`. Xenon Technology leads to itself and is never offered.

The names are in TECHNAME.LBX's first entry: names ending in a zero byte,
with no header. First the fields by id (the hyper-advanced fields at the end
carry their category's name, such as "Biology"), then "No Tech" as
application 0 and the applications by id.

## The start

With average technology a player knows Starting Tech and the fields of
`prewarp_techfield` (Engineering, Nuclear Fission, Chemistry, Physics,
Electronics, Cold Fusion), with all their applications. Measured with the
game's own field states in a new game: these are known, and the next field
of each category can be researched.

## A turn

When the turn ends without a project the original first shows the Select
New Research screen. The turn then runs:

- The stardate moves on a tenth of a year (3500.0, 3500.1, ...).
- The treasury gets the income the side panel shows (measured: 50, 58, 66,
  74 BC with +8 BC).
- The project gets the research points of all the player's colonies (see
  [economy.md](economy.md)).

The side panel shows about how many turns the project still takes: until
one and a half times its cost, rounded up (measured: Advanced Engineering,
80 RP, at 10 RP a turn shows ~11, ~10 and ~9 turns after the first three
turns).

A field is learnt for certain once twice its cost is put in;
`fixed_research_cost` 0 (the default) allows an earlier breakthrough from
its cost on. Assumed until measured: the chance grows evenly from nothing at
the cost to certain at twice the cost. After a breakthrough the points are
gone, unless `keep_research_on_breakthrough` is 1: then what is over twice
the cost is kept. A creative race learns all of a field's applications; the
others learn the one chosen on Select New Research (measured: the
application row clicked is the one learnt), else the field's first.

## Select New Research

Measured on the Alkari's screen:

- The race's scientist, SR_R*n*_SC.LBX entry 0 (*n* the race in hex, 0
  Alkari to C Trilarians), a 640 × 480 animation whose frame 0 is whole;
  TECHSEL.LBX entry 0 over it at x 161. The colours start from FONTS.LBX
  entry 7's palette (its pointers are this screen's), then the scientist's
  (0 to 148), the panel's (149 to 191) and TECHSEL.LBX entry 14's (224 to
  239), which the texts use. How fast the scientist moves is assumed (a
  frame every two ticks).
- Eight boxes, 218 × 82, at x 174 and 401, y 48, 153, 258 and 365:
  Construction, Power; Chemistry, Sociology; Computers, Biology; Physics,
  Force Fields. The category names above them are part of the panel.
- In each, the field's name in font 4 at (box + 2, box + 3), the top right
  box one pixel lower; the applications in font 3 at x box + 12, from the
  name's top + 19, 15 apart; the cost ("80 RP") in font 3 ending at box + 215
  (with its spacing), 19 above the name. Two pixels between letters, shade 1
  in colour 225, shades 2 and 3 in 228.
- The box under the pointer is lit: its name and cost in colour 231, and a
  one-pixel border round the box. The application under the pointer is lit
  too, with an arrow to it: from 19 pixels into the box down at x box + 3
  to 5 below the application's top, then right 7 pixels, with a head 3
  pixels each way. The border and the arrow change between (125, 219, 20)
  and (158, 251, 28); how fast is assumed.
- A click on a box takes its field, and the turn runs. The application the
  pointer is on is the one learnt, for a race that isn't creative.

**Added in Hegemon:** the arrow keys or the D-pad light the next box, Page
Up and Page Down or the gamepad's shoulder buttons point at the lit box's
next application, and Enter or A takes it.

## Research done

When a field is finished at the end of the turn the original shows the
application learnt, before Select New Research asks for the next field.
Measured on the Alkari's screen with Reinforced Hull:

- The colours: the scientist's (0 to 148), the application's (149 to 191)
  and SCIENCE.LBX entry 0's (192 to 255), which the beam and the texts use.
- Drawn in this order, each a 640 × 480 animation drawn at (0, 0): the
  race's scientist (as on Select New Research), the beam of light
  (SCIENCE.LBX entry 2, 13 frames), the application turning
  (TANM_*nnn*.LBX entry 0, *nnn* the tech's number in three digits, 30
  frames) and the glass panel (SCIENCE.LBX entry 1, 15 frames). Each frame
  is drawn over the ones before it.
- All four move a frame each tick (about 18 a second). Past its last frame
  the panel starts again at frame 5, the number in its header; the others
  start again at their first.
- The name and description come from HELP.LBX entry 0, a text table of 707
  records of 1403 bytes: record *n* is tech *n*, with its name at the start
  and its description from byte 103 on. The name is in font 3 at y 45, the
  description in font 2 from y 59, 13 pixels a line, both centred on x 310,
  shade 1 in colour 240 and shade 2 in 246.
- The message is BILLTEXT.LBX entry 6 (entry 11 holds the same), "Your
  scientists have completed their research in" and the field's name in place
  of byte 0x89, then "!". It is in font 4 from y 420, 17 pixels a line,
  centred on x 337, shades 1 to 3 in colours 242, 245 and 244, with a black
  border a pixel wide all round the letters.
- Two pixels between letters. Where the lines break is assumed: the
  description at 180 pixels and the message at 380 (seen: 172 fits and 186
  doesn't; 371 fits and 390 doesn't).
- Assumed: a click or any key goes on, and a creative race sees a screen for
  each application.

## Still to do

The breakthrough chance, the research done screen's opening and sounds, the research screen of the colonies, other
technology levels at the start, the hyper-advanced fields, the computer
players' research, and the Select New Research screen's sounds.
