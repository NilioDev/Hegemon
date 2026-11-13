# The fan patch's rule settings

Fan patch 1.51 lists every rule setting it knows in `150\docs\config.json`,
with its default and the values it allows: 426 settings in 1.51, from the
number of stars per galaxy size to the weapons' data. Hegemon reads its
rules from that list, so a newer fan patch with new defaults or new
settings needs no change in Hegemon. The settings files (see
[config.md](config.md)) then change the defaults.

## The list

The file is JSON: an object with `patch_version` ("1.51") and
`parameters`, a list of settings. Each setting has a `name` and a `type`:

- `integer`, with `min` and `max` and a number as `default`;
- `enum`, with its words in `values` (their numbers count from 0) and a
  word as `default`;
- `string`, a text or a word;
- `table`, with `columns` (each a name and one of the types above, or
  `mask` or `ratio`), `rows` (a list of row names for each dimension;
  empty for a table of one row) and `default`: one list per dimension
  nested in each other, then the columns. A table of one column leaves out
  the innermost list.

Other members, such as `class`, `ctype`, `size` and `comment`, describe how
the fan patch keeps or explains the setting; Hegemon doesn't need them.

For example, `star_class_chance` has the rows blue, white, yellow, orange,
red, brown and black_hole, the columns young, average and old, and the
default `[[20, 10, 5], [25, 15, 5], ...]`.

## Settings that change it

A setting in the settings files whose first name is in the list changes
it:

```
star_special_chance = 50;                 # a single value
stars_in_galaxy medium = 40;              # one cell of a table of one row
star_class_chance white = 1 2 3;          # a whole row
star_class_chance blue young = 7;         # one cell
```

After the name come the row names, one for each dimension, then
optionally a column name. Without a column, the setting gives every
column of the row. Names are matched in any case. A number for an
`integer` may have letters before it, as in `sd3`. Hegemon refuses an
unknown row or column, the wrong number of values, a number outside `min`
to `max` and an `enum` word that isn't allowed. Settings the list doesn't
know, such as the loader's own `scan_mods` and `include`, are left to the
loader.

Masks and ratios are kept as written until a rule that uses them is
built.
