# Fonts

The original's bitmap fonts are entry 0 of `FONTS.LBX` (and of
`IFONTS.LBX`). The entry holds six fonts, numbered 0 to 5, followed by the
pixels of every glyph. All numbers are little-endian. Worked out by drawing
the glyphs and matching them against captures of the original's screen.

## Tables

| Offset | Size | What |
| --- | --- | --- |
| `0x56C` | 2 × 6 | Height of each font in pixels (6, 9, 12, 13, 15 and 23 in `FONTS.LBX`) |
| `0x59C` | 256 × 6 | Width in pixels of each of the 256 characters, per font; 0 means the font has no such character |
| `0xB9C` | 1024 × 6 | Where each character's glyph starts, 4 bytes per character, per font; counted from the start of the glyph data |
| `0x239C` | rest | Glyph data |

The bytes between the start of the entry and `0x56C` are not needed for
drawing; what they hold is still to be measured.

A glyph runs from its own start to the start of the next character's glyph.

## Glyphs

A glyph is read byte by byte, from the top left:

| Byte | Meaning |
| --- | --- |
| `0x00` to `0x7F` | One pixel of this shade; 0 is see-through |
| `0x80` | Start the next row |
| `0x81` and up | Skip (byte − `0x80`) see-through pixels |

Rows start empty at the top, so a lower-case letter starts with a few `0x80`
bytes. Hegemon cuts a row at the character's width and stops after 64 rows,
so damaged data can't draw outside the glyph's box.

## Drawing text

Characters follow each other left to right: each takes its width plus the
spacing (1 pixel for all text measured so far).

A shade is not a colour: the screen that draws the text chooses which
palette colour each shade gets, and whether there is a shadow. The shadow is
drawn first, in its own colour, at fixed offsets from every drawn pixel; the
glyphs then go on top. The usual shadow is one pixel right and one down; the
credits draw a one-pixel outline all round instead (see
[../rules/main-menu.md](../rules/main-menu.md)).
