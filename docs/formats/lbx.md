# LBX archives

Nearly all of the original game's data sits in files with the `.LBX`
extension: pictures, animations, sounds, music, fonts, texts and rule tables.
An LBX file is a plain container of numbered entries, without names, without
compression and without folders. What an entry holds depends on the archive;
those formats get their own pages.

All numbers are little-endian.

## Header

The header is always 2048 bytes long, and the first entry starts right after
it.

| Offset | Size | What |
| --- | --- | --- |
| 0 | 2 | Number of entries, `n` |
| 2 | 4 | Signature, the bytes `AD FE 00 00` |
| 6 | 2 | A number that tells which kind of data the archive holds; 0 in every archive of version 1.31 |
| 8 | 4 × (`n` + 1) | Where each entry starts, counted from the start of the file, followed by where the last entry ends |
| … | | Up to byte 2048: not used |

Entry `i` runs from offset `i` up to offset `i + 1`. The offsets never go
down, so an entry can be empty (two equal offsets; 38 of them in version
1.31). The last offset is the size of the file.

The unused part of the header is not zero in most archives. It holds left-over
bytes from the tool that made them and carries no meaning; Hegemon ignores it.
There are no entry names in it.

With 2048 bytes of header, at most 509 entries fit. The largest archive of
version 1.31 has 498.

## Files that are videos

Ten `.LBX` files of version 1.31 are not archives at all but Smacker videos:
the opening (`INTRO.LBX`) and the endings and other film sequences, such as
`WININFIN.LBX` and `LOSERFIN.LBX`. They start with the Smacker signature
`SMK2` instead of the LBX header. Hegemon tells them apart by the signature,
not by the name.

## What Hegemon checks

The reader refuses a file when it is shorter than the header, when the
signature is missing, when it lists more entries than the header holds, or
when an offset points back before the previous one, into the header, or past
the end of the file.
