# OpenDSP audio port

This branch migrates ctrHexenII's audio output to the embedded OpenDSP
firmware. The game still performs its original Quake sound mixing on ARM11;
OpenDSP handles playback/resampling and optional reverb/delay on the Teak DSP.

## Build

Install devkitPro devkitARM and libctru, then build/install
[OpenDSP](https://github.com/Cellenseres/OpenDSP) with
`make -C /path/to/OpenDSP/library install` before running `make` here.
The old vendored 2016 headers and `lib/libctru.a` are not used.

## Controls

See [the 3DS control guide](docs/CONTROLS.md) for button mappings and resetting
saved game controls.

## Options

Console variables (disabled by default):
- `h2_reverb 1` adds DSP reverb to the mixed stereo output.
- `h2_echo 1` adds DSP delay/echo to the mixed stereo output.

The sample ring is 16,384 stereo PCM frames (65,536 bytes). The original
backend allocated only 16,384 bytes while describing a larger buffer.
Playback position is now obtained from NDSP rather than wall-clock ticks.
Cache flushing only touches newly mixed regions.

CSND remains as a fallback if OpenDSP cannot initialize.

## CPU offload roadmap

The game still mixes individual sound effects in `source/snd_mix.c`, which
runs on ARM11. To move that cost to the DSP, the next change needs to map
Quake's dynamic and ambient channels to separate NDSP voices while retaining
fallback for >24 simultaneous sources and keeping sound cache lifetimes safe.
Effects currently process the combined output, not individual game emitters.

Build and hardware tests are required before claiming working audio.
Keep the original GPL licensing and do not distribute proprietary game data.

## SD card layout

Keep all game files under one folder; no `/data1` or `/portals` in the SD root:

```text
sdmc:/3ds/ctrHexenII/
    ctrHexenII.3dsx
    data1/
        pak0.pak
        pak1.pak
    portals/               # optional Portal of Praevus mission pack
        pak0.pak
```

Hexen II loads the official game data from `data1` and only adds `portals`
if that folder exists. Configs and saves go into the active game directory
(`data1` without the expansion). Do not redistribute commercial game assets.

The configured base directory is `sdmc:/3ds/ctrHexenII`, not the SD root.

## Startup configuration compatibility

The original retail `hexen.rc` / `default.cfg` startup scripts are executed from
`data1`. Some PC-only console commands in them are not implemented by the 3DS
port and may produce harmless `Unknown command` messages in the debug console.
This branch supports the original `ENTER` and `ESCAPE` key aliases and keeps
script boundaries separated even when the file has no final newline.

No external `dspfirm.cdc` is required, and the port does not probe for that
file. `opendspInit()` uses the firmware built into the OpenDSP static library.
The CSND fallback is only used when OpenDSP initialization itself fails.

## Sleep/wake diagnostics

The game registers an APT wake hook that only records the event. After
`aptMainLoop()` returns, the main loop skips one simulation frame, rebases
the frame clock and reacquires the software framebuffer pointer.

GitHub Actions publishes two builds from the same source:
- `ctrHexenII-3dsx`: normal OpenDSP build.
- `ctrHexenII-sleep-noaudio`: diagnostic build that injects `-nosound`
  before game startup, never initializing game audio services.

Try both after closing and opening the 3DS lid, at the menu and in gameplay.
If only the normal build freezes, investigate NDSP/OpenDSP wake handling or
the game's wave-buffer state. If both freeze, investigate APT, graphics and
other game code. Successful cross-builds do not prove sleep/wake correctness
on hardware.

## HOME resume

APT uses different lifecycle hooks: `APTHOOK_ONRESTORE` for HOME-menu
restoration, `APTHOOK_ONWAKEUP` for lid sleep. After `aptMainLoop()`
returns, the game resets the frame delta, refreshes the framebuffer,
clears stale sound paints and requeues its *existing* looping NDSP wavebuf
via `SNDDMA_Resume()`. The embedded firmware is not reloaded by the game.
The CSND fallback likewise restarts its PCM ring.

This is a **candidate** fix for silence after HOME. It does not claim
to resolve permanent lid-sleep black screens. The independent no-audio
build also failed on actual hardware, so the remaining sleep issue must
be investigated separately from OpenDSP.
