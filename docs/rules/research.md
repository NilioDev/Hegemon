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
the cost is kept. Until the screen that offers the applications is made, a
creative race learns all of a field's applications and the others its first.

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
- A click on a box takes its field, and the turn runs.

## Still to do

The screen that offers a field's applications after a breakthrough, the
breakthrough chance, the research screen of the colonies, other
technology levels at the start, the hyper-advanced fields, the computer
players' research, and the Select New Research screen's sounds.
