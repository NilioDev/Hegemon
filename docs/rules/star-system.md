# Star systems

The window that shows a star system from above. Measured on version 1.51 of
the fan patch: screenshots of the window after a click on a star of the
galaxy map (with every star made explored through the Lua interface's
`set_stars`), and of the window the original shows when ships arrive, with
the stars and planets read from the Lua interface.

## When it opens

- A click on a star of the galaxy map (while no fleet window is open) shows
  the window at the map's top left corner, titled "Star System %s"
  (HESTRNGS.LBX message 362), or "Star System Unexplored" (363) for a star
  the player's ships have not been to: then only the star is shown
  (assumed).
- When the player's ships reach a star for the first time, the window opens
  in the middle of the map, titled "Scouts arrive at the %s system"
  (ESTRINGS.LBX message 456). Hegemon shows these before the turn's other
  screens (the order is assumed).
- When ships with a colony ship arrive, Close then opens it once more,
  asking where the ship lands (see [colonizing.md](colonizing.md)).

Close (or Esc) closes it. Hegemon also takes Enter or Space, and on a
gamepad A or B.

## The window

All BUFFER0.LBX, from the corner of the frame:

- The frame, entry 73 (347 × 273), at (22, 22) after a click and at
  (147, 104) on arrival. It holds the stars behind the system and the bar for
  the fleets at the bottom.
- The asteroid belts: entry 91, frame = orbit, at (29, 59).
- For every orbit with a planet or gas giant its ellipse, entry 77 + orbit
  (288 × 154), at (29, 59). The middle of the ellipses is (172.5, 135.5);
  orbit n's line is 24 × (n + 2) − 1.5 pixels left and right of it and
  13 × (n + 2) − 1.5 above and below.
- The star, entry 83 + colour (blue to brown, then the black hole's), at
  (156, 120).
- The planets, entry 92 + 5 × climate + size (toxic to gaia, tiny to huge),
  and gas giants, entry 142 (assumed): pictures of 31 × 31 with the disc in
  their top left corner, its middle on the orbit's line. They turn: a frame
  every 7/60 second (39 or 40 frames).
- Where on the orbit a planet is: the measured planets are at all sorts of
  places, and nothing in the savegame's planet or star records keeps it.
  Hegemon puts each at an angle made from the star's and orbit's numbers
  until the original's rule is found.
- The planets' shading: entry 143 + size (31 frames round the star, frame
  angle × 31 / 360 counted against the clock from the right), each pixel a
  level from 1 (light) to 9 (dark), drawn over the planet. A shaded pixel
  is the nearest colour to the planet's own times the level's factor:
  1, 0.995, 0.92, 0.85, 0.78, 0.70, 0.61, 0.52, 0.45 for levels 1 to 9
  (fitted: about nine in ten of the measured pixels exactly).
- The planets' colours: each planet picture brings its own first 22. The
  original loads them into the screen's colours, so planets keep their own
  tint (a barren planet's greenish greys); Hegemon draws the nearest of the
  map's colours instead, a little greyer.
- The player's fleets at the star: entry 273, frame = banner, at (18, 241)
  (the other entries for other fleets are still to be measured).
- Close: entry 82, at (263, 236).
- The title in the bar on top, centred on x 172 (left = 172 − (width − 2) /
  2): font 4 with two pixels between letters at y 19, or, when that is too
  wide (Hegemon: over 300 pixels), font 3 with one pixel at y 20; shades 1,
  2 and 3 in colours 60, 65 and 64.
- A wormhole there: "Wormhole links %s" (HESTRNGS.LBX message 364) when the
  player has explored the star at its other end, "Stable Wormhole" (365)
  otherwise; font 1, one pixel between letters, shade 1 colour 110 and
  shade 2 colour 115, its last pixel at x 328 and its top at y 219.
