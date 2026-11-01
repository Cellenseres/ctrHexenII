# OpenDSP audio port

This branch migrates ctrHexenII's audio output to the embedded OpenDSP
firmware. The game still performs its original Quake sound mixing on ARM11;
OpenDSP handles playback/resampling and optional reverb/delay on the Teak DSP.

## Build

Install devkitPro devkitARM and libctru, then build/install
[OpenDSP](https://github.com/Cellenseres/OpenDSP) with
`make -C /path/to/OpenDSP/library install` before running `make` here.
The old vendored 2016 headers and `lib/libctru.a` are not used.

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
