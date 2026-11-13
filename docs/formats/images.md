# Pictures and palettes

Most entries in the LBX archives (see [lbx.md](lbx.md)) are pictures: a still
picture, an animation, or a set of pictures for one thing such as a button
in its states. Each pixel is an index into a palette of 256 colours. All
numbers are little-endian.

Version 1.31 holds 6,532 pictures across its archives.

## Header

| Offset | Size | What |
| --- | --- | --- |
| 0 | 2 | Width in pixels |
| 2 | 2 | Height in pixels |
| 4 | 2 | Always 0 |
| 6 | 2 | Number of frames, `n` (1 for a still picture) |
| 8 | 2 | A frame number. The opening animation of the main menu (50 frames) has 49 here; what it does is still to be measured |
| 10 | 2 | Flags, see below |
| 12 | 4 × (`n` + 1) | Where each frame starts, counted from the start of the entry, followed by where the last frame ends |
| … | | The palette part, when flag `0x1000` is set |

## Flags

| Bit | Meaning |
| --- | --- |
| `0x0100` | The frames are plain rows of pixels (see below). 43 pictures, all small. |
| `0x1000` | The entry carries colours for the palette. |
| others | Seen: `0x0400`, `0x0800`, `0x2000`. Their meaning is still to be measured. |

## Palette part

Right after the frame offsets, when flag `0x1000` is set:

| Size | What |
| --- | --- |
| 2 | First colour to set |
| 2 | Number of colours, `c` |
| 4 × `c` | One entry per colour: one unused byte, then red, green and blue |

Colours have 6 bits per channel (0 to 63), as the VGA hardware of the time
took them. Hegemon widens them to 8 bits by shifting left by 2 and repeating
the two highest bits in the lowest two, so 0 stays 0 and 63 becomes 255. The
full-screen pictures, such as the main menu's, carry all 256 colours.

## Frames made of runs

Most frames describe only the pixels they draw, as runs along the lines.
Pixels outside the runs are not touched: that is how parts of a picture are
transparent, and how an animation frame paints only what changed since the
frame before.

A frame starts with the word 1, then the line to start on. After that comes a
list of word pairs (`count`, `step`):

- `count` above 0: move `step` pixels to the right on the current line, then
  `count` pixel bytes follow and are drawn from there; the position moves on
  past them. When `count` is odd, one byte of padding follows, so the next
  pair starts on an even byte.
- `count` 0 and `step` 1000: the frame ends.
- `count` 0 and any other `step`: go down `step` lines and back to the left
  edge.

A line can hold several runs with gaps between them; each `step` is counted
from the end of the run before it, not from the left edge.

## Frames made of plain rows

When flag `0x0100` is set, a frame is simply width × height pixel bytes, row
after row, sometimes followed by a few bytes of padding.

## What Hegemon checks

A picture is refused when the header is cut short, the width or height is 0,
the reserved word is not 0, there are no frames, an offset points back or
past the end of the entry, the palette part runs past colour 255, a frame of
runs lacks its start word or its end, a run would land outside the picture,
or a frame of plain rows is shorter than the picture.
