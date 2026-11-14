# Fleets

How ships travel between the stars, and how the galaxy map and the fleet
window show it. Measured on version 1.51 of the fan patch: the ships of every
player read each turn from the original's Lua interface (`get_ships`: place,
destination, turns left, speed), and screenshots of the map and the fleet
window.

## Fleets

A player's ships at one star with no orders are one fleet; ships sent from
there to the same star are another, and ships on their way together stay one.

## Speed

A drive's speed is the drive table's `warp_speed` in parsecs a turn (the
nuclear drive: 2). A parsec is 30 map units. A fleet goes at the speed of its
slowest ship. Ships of the player's designs have their design's drive;
assumed for now: the starting ships and the colony, outpost and transport
ships have the drive of the player's first design.

## A turn's way

At the end of the turn every ship with an order goes towards its star (fits
all 36 moves measured, of every player's scouts and colony ships):

- d is the whole part of the square root of dx² + dy², from where the ship
  is to the star.
- If d is no more than the step (speed × 30), the ship is at the star.
- Otherwise x goes |dx| × step / d, rounded up, towards the star, and y the
  same with dy.

Ships sent off in a turn leave at its end; they keep their place at the star
until then (the original marks them as leaving). A ship arriving at a star
has explored it.

The ETA, in the fleet window and under a fleet on the map, is d divided by
the step, rounded up.

## On the galaxy map

The human's fleets, in BUFFER0.LBX entry 208 − scale (12 × 10 on a medium
map):

- At a star with no order: at the star picture's upper right (see
  [galaxy-map.md](galaxy-map.md)).
- Sent off but still at the star: at its upper left, the marker's corner 4
  pixels left of and 2 above the star picture's corner (measured on a medium
  map with one star, sent in two directions).
- On its way: the marker centred on its place, (x − 6, y − 5) for 12 × 10;
  under it "eta N" (HESTRNGS.LBX message 307) in font 1, one pixel between
  letters, shade 1 colour 74 and shade 2 colour 79 for the red banner, no
  outline, its left 7 pixels left of the marker's and its top right under
  it.
- The course: a line from the marker's middle (corner + (6, 5)) to the
  star's, a pixel each step along the longer side, under the stars. Its
  pixels go in eights: 110, 110, 111, 114, 116, 116, 114, 111 (dark green
  to light), and the pattern moves a pixel towards the star every 1/15
  second.
- Explored stars that are not the home star have their names in grey:
  shade 2 colour 12, shade 1 colour 1, with the same outline as the home
  star's.

While the window shows a fleet the original makes its marker light up and
move through its frames; Hegemon doesn't do that yet.

## The fleet window

A click on a marker opens it at the map's upper left. From the top, all
BUFFER0.LBX:

- Entry 52 (198 × 35) at (22, 22): the title, "%s Fleet" (HESTRNGS.LBX
  message 102) with the race's name (ESTRINGS.LBX message 645 + race, such
  as "Alkari"), in font 4 with one pixel between letters, centred on x 121
  (left = 121 − width / 2) with its top at 36; shades 1, 2 and 3 in
  colours 111, 115 and 113, with the map's dark outline (shade 1 in colour
  1, the others in 2).
- Entry 54 (198 × 169, three rows of three boxes) from (22, 57), cut to
  the rows the fleet needs, 56 pixels each. The boxes are 54 × 53 from
  (37, 60), 58 apart; a box whose ship is chosen has entry 11 behind the
  ship. The ship is SHIPS.LBX entry 50 × banner + picture, 1 right of and 2
  below the box's corner, with the colours from 192 on of entry
  50 × banner + 49. The picture: the design's, 45 for a colony ship, 46 an
  outpost ship, 47 a transport.
- Entry 56 (198 × 37) under the boxes: All (entry 60, frame 1 under the
  pointer) 17 right of and 4 below its corner, and the status 10 below its
  top, centred on x 150, in font 3 with two pixels between letters, shades
  1 and 2 in colours 112 and 115: "Orbiting %s" (message 103) or "ETA %d
  turn"/"ETA %d turns" (107, 108).
- Under it the buttons, 195 × 32 from x 24: Close (entry 72, frame 1 under
  the pointer); for a fleet with a colony ship at a star the original shows
  Colonize Planet (entry 69) under Close.

This draws the measured window to the pixel but two of its frame's.

All the boxes are chosen when the window opens. The window stays open over
the turn, showing the fleet where it is then.

A click on a box changes
whether its ship goes, All chooses them all, Close closes it. A click on a
star sends the chosen ships there: the window then shows those ships with
their ETA and no box chosen, and the Colonize Planet button goes.

## Added in Hegemon

- Clicking a star while no box is chosen sends the whole fleet.
- Keyboard and gamepad: Tab (Shift+Tab) or the shoulder buttons open the
  next (previous) fleet; with the window open the arrow keys or the D-pad
  move the pointer to the nearest star that way, and Space or A sends the
  fleet there; Esc or B closes the window.

A fleet reaching a star the player has not been to shows the star system
(see [star-system.md](star-system.md)).

Not done yet: landing a colony ship (the original asks where at once),
fuel range, other players'
fleets on the map, more than nine ships in a fleet (the original's window
then has a scroll bar), and the other buttons.
