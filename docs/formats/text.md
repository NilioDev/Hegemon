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

## Lists of texts

`HESTRNGS.LBX` has one entry with a single record of 13521 bytes. That record
holds about 400 texts, each ending at a zero byte, numbered from 0 in their
order. Text 242 is "Generating Universe ..." and text 161 is "Placing home
worlds ...". Hegemon refuses such a list when the record runs past the end
of the entry. The zeros after the last text don't count as texts.

## Help texts

`HELP.LBX` entry 0 is a text table of 707 records of 1403 bytes. Each record
has a name at its start and a longer text from byte 103 on, each ending at a
zero byte; the bytes between them are still to be measured. Record *n*, for
*n* from 1, is tech *n* (record 56 is Reinforced Hull); record 0 is "No
Tech". The fan patch has its own copy in `lbx/en`.

## Messages

`BILLTEXT.LBX` has 468 entries, each a text table with one record of 100
bytes. Byte 0x89 in a text stands for a name filled in when it is shown.
Entry 6 is the message that a field's research is complete.
