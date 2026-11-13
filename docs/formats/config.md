# Fan patch settings

The community fan patch 1.51 (`ORION150.EXE`, from moo2mod.com) reads most
of its rules from plain-text settings files in the game folder. Hegemon
reads the same files the same way, so a change made there, by the player or
by a newer release of the fan patch, works in Hegemon too. The meaning of
each setting is documented by the fan patch itself, in
`150\docs\PARAMETERS.CFG` and its manual; this page covers how the files are
read.

The patched game refuses to start without `ORION2.CFG` ("unable to open
ORION2.CFG"); Hegemon then asks the player to install the fan patch.

## Statements

A file is a list of statements, each ending in `;`. Line breaks don't
matter, `#` starts a comment up to the end of the line (outside a text),
and names are matched without regard to case.

- **Settings**: one or more names, `=`, and one or more values:

  ```
  warn_blockers = 0;
  event_delay comet = 150;
  format version = "Version $VERSION$" 1 r 541 433;
  ```

  A value is a text in double quotes (no escapes), a number (decimal,
  possibly negative, or hexadecimal with `0x`) or a word, such as `r`,
  `intact` or a path. Settings are kept in the order they are read. For most
  of them the last one counts; a few, such as
  `newgame_postprocessor_script`, are written several times in the fan
  patch's own files, and then each of them counts. Many settings use `0`
  for "not set".

- **Commands**: a name and its arguments, without `=`:
  - `include path;` reads another file right there.
  - `enable mod_id;` enables a mod (see below).

## Paths

Paths use `\`. A path that starts with `\` starts at the game folder; any
other path starts at the folder of the file it is written in. `..` goes up a
folder. A path that starts with `?` is optional: when the file isn't there,
the statement is skipped; without `?` a missing file is an error. `$LANG$`
stands for the language of the game, `EN` for the English one. Hegemon
matches every part of a path without regard to case, never follows links,
and refuses a path that leaves the game folder.

A setting named after an LBX archive in lower case, such as
`combat.lbx = ..\..\lbx\COMBAT.LBX;`, makes the game read that file instead
of the archive of that name in the game folder.

## Mods

`scan_mods = \150\mods;` names the folder with the mods. Every `.CFG` file
one folder below it (`150\mods\*\*.CFG`) that sets `mod_id` and `mod_class`
is a mod:

| Setting | What |
| --- | --- |
| `mod_id` | The name `enable` uses. Two mods with the same id are both left out. |
| `mod_name` | The name shown to the player (`$MOD$`); the id when missing. |
| `mod_class` | Mods of one class are alternatives: only the first one loaded is. Usual classes: `core`, `ai`, `map`, `ruleset`; a class starting with `_` is hidden. |
| `mod_order` | Lower numbers load first. Without it: 0 for `core`, 100 for `ai`, 200 for `map`, 300 for `ruleset` and 1000 for others. |

After `ORION2.CFG` and its includes are read, the enabled mods are read one
by one, the lowest `mod_order` first (ties in the order of their folders and
file names). A mod can enable more mods. Out of the box `ORION2.CFG` includes
`150\ENABLE.CFG`, which enables the chosen core mod (`150`, "1.50 standard"),
and enables `USER`, a hidden mod with order 2000 that includes the player's
own `150\USER.CFG` last.

## Limits

To keep damaged files from running away, Hegemon reads files of at most
1 MiB, at most 1,024 files, and includes at most 16 deep.

## Settings Hegemon uses so far

| Setting | Where |
| --- | --- |
| `<name>.lbx = <path>` | Which file replaces an LBX archive |
| `format version`, `format mod_name` | [../rules/main-menu.md](../rules/main-menu.md) |
| `stock_race` | [races.md](races.md) |
