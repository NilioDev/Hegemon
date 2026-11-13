# Text tables

Some LBX entries hold a list of texts of the same size, such as the credits
of the main menu (entry 0 of `CREDITS.LBX`). All numbers are little-endian.

| Offset | Size | What |
| --- | --- | --- |
| 0 | 2 | Number of records, `n` |
| 2 | 2 | Size of each record in bytes, `s` |
| 4 | `n` × `s` | The records |

A record's text ends at its first zero byte; the bytes after it are not
used. Hegemon refuses a table whose records would run past the end of the
entry.

## The credits

In `CREDITS.LBX` the first record is the number of credit lines, written in
digits. Then each line takes two records: the text of the left column and
that of the right column. Either may be empty.

## Dialog titles

`KENTEXT1.LBX` holds the English texts of the dialogs, one table per entry,
each with a single record of 100 bytes. Entry 156 is "Enter Ruler Name"
(entry 161 holds the same text). The other languages' files have not been
looked at yet.
