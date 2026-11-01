# CIA packaging for ctrHexenII

The `.cia` and `.3dsx` both link the same embedded OpenDSP firmware. The CIA
is a proper installed HOME Menu application, not a converted 3DSX, and does not
require `sdmc:/3ds/dspfirm.cdc`.

## Build and install

Run `make` for a 3DSX, or `make cia` for both formats. The CIA target follows
OpenDSP Sound Lab's recipe: `bannertool makebanner` then `makerom -f cia`
with the built ELF, SMDH and dedicated RSF file.

GitHub Actions publishes separate normal and no-audio diagnostic artifact
bundles; each contains a CIA and a 3DSX. Both CI variants have the **same**
title ID. Install only one variant at a time; installing one replaces the other.

Install the CIA using FBI, then launch from the 3DS HOME Menu. No need to
run the Homebrew Launcher.

## Game files

Keep legally obtained game data outside the CIA at:

```text
SD:/3ds/ctrHexenII/
├── data1/
│   ├── pak0.pak
│   └── pak1.pak
└── portals/           # optional mission pack
```

The CIA stores no commercial PAK files. The 3DSX may live beside the data in
`SD:/3ds/ctrHexenII/ctrHexenII.3dsx`, but the installed CIA does not need
this executable on the SD card.

## Metadata and permissions

The CIA RSF is derived from `OpenDSP/examples/soundlab/meta/soundlab.rsf`
and uses test title ID `000400000F0D5900` / `CTR-P-HXOD`.
These are **not** the original author's 2016 CIA identifiers. This avoids
accidentally replacing the original Hexen II port or Sound Lab.

## Testing

Try the normal CIA in gameplay and menu: close lid, reopen, and compare with
the 3DSX on identical hardware. Repeat with the no-audio CIA if needed.

If only the 3DSX freezes, investigate hbmenu/Rosalina entry conditions.
If both CIAs and 3DSX builds freeze, investigate common APT/GSP/game code.
An installed CIA is a diagnostic comparison, not a proven sleep fix.

Also check whether Rosalina InputRedirection is enabled. Historical
Luma3DS issue #904 documents lid-wake black screens with that enabled.
