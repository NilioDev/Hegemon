# gamedata/

Put your own copy of the original 1996 game files here. Everything in this
folder except this README is ignored by Git and must never be committed.

The easiest way is to copy the complete game folder (the one with the
`.LBX` files, such as `MAINMENU.LBX` and `TECHNAME.LBX`) into this folder, so
that those files sit directly in `gamedata/`. Both the DOS and the Windows 95
installation work. File names are matched without regard to case, and other
files are ignored.

Tests that need the original files skip when they are not here.
