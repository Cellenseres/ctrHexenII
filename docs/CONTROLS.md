# ctrHexenII controls (OpenDSP branch)

The 3DS buttons are sent to the original Hexen II input system by
`source/sys_ctr.c`; `source/in_ctr.c` handles analogs and touch.
The listed **default bindings** are installed only if no `config.cfg`
is detected in the active game directory. Custom configs can override them.

| 3DS input | Engine key or behavior | Default action |
| --- | --- | --- |
| R | `r` | Attack |
| Y | `y` | Attack |
| A | `a` | Jump |
| B | `b` | Jump |
| X | `x` | `impulse 10` (change weapon) |
| L | `l` | Crouch |
| D-pad up | `UPARROW` | Show info |
| D-pad down | `DOWNARROW` | Use inventory |
| D-pad left/right | `LEFTARROW` / `RIGHTARROW` | Inventory navigation |
| START | `ENTER` | Enter key; not a default attack button |
| SELECT | `ESCAPE` | Menu/back |
| ZL / ZR | `k` / `t` | No forced default bindings |
| Circle Pad vertical | movement | Forward/back |
| Circle Pad horizontal, New 3DS | movement | Strafe |
| Circle Pad horizontal, old 3DS | camera | Look left/right |
| C-Stick, New 3DS | camera | Look around |
| Touchscreen drag | camera | Look around |

In the menus, A confirms and B usually goes back. Holding L while launching
enables the debug console; holding R while launching disables the game console
display (this is separate from the in-game R = attack mapping).

## Restoring controls

The startup script `hexen.rc` executes the game's PC `default.cfg`.
The port then applies its 3DS default bindings, *but only when there is no
saved `config.cfg`*. Older user bindings may therefore override these
defaults. Use **Options → Reset to Defaults** or back up and rename
`config.cfg` from `sdmc:/3ds/ctrHexenII/data1/`, or from
`sdmc:/3ds/ctrHexenII/portals/` when the expansion is active, then restart.

Config commands can also be entered in the game console:

```text
bind r +attack
bind y +attack
bind a +jump
bind b +jump
bind x "impulse 10"
bind l +crouch
```

Some original PC-only configuration directives are not implemented by this
3DS port and may report `Unknown command`. A series of corrupted-looking
commands such as single letters or `abyx` is **not** a normal key mapping:
report the entire log and the `hexen.rc`/`default.cfg` used.
The command-buffer overlapping-copy bug is fixed in this branch, but no
specific retail configuration has yet been reproduced on 3DS hardware.
