/*
Copyright (C) 2015 Felipe Izzo
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <stdio.h>
#include <3ds.h>
#include <opendsp.h>
#include <opendsp/effects.h>
#include "quakedef.h"

#define TICKS_PER_SEC 268111856LL
#define QUAKE_SAMPLERATE 32000
#define CSND_SAMPLERATE 32730
#define CSND_BUFSIZE 16384
#define DSP_BUFSIZE 16384
qboolean isDSP = false;
u32 SAMPLERATE = QUAKE_SAMPLERATE<<1;
u8 *audiobuffer;
ndspWaveBuf* waveBuf;
u64 initial_tick;
int snd_inited;
static u32 last_submitted;
static u32 capabilities;
static int reverb_active = -1, echo_active = -1;
static int effects_registered;
static cvar_t h2_reverb = {"h2_reverb", "0", true};
static cvar_t h2_echo = {"h2_echo", "0", true};
void SND_ResetSoundClock(void);

// createDspBlock: Create a new block for DSP service
void createDspBlock(ndspWaveBuf* waveBuf, u16 bps, u32 size, u8 loop, void* data){
	waveBuf->data_vaddr = (void*)data;
	waveBuf->nsamples = size / bps;
	waveBuf->looping = loop;
	waveBuf->offset = 0;	
	DSP_FlushDataCache(data, size);
}


/* Entire stereo pre-mix sent to DSP effects; no CPU AUX callback. */
static void update_effects(void)
{
    int rev = h2_reverb.value > 0 && (capabilities & OPENDSP_CAP_REVERB);
    int echo = h2_echo.value > 0 && (capabilities & OPENDSP_CAP_DELAY);
    float mix[12] = {0};
    if (!isDSP) return;

    if (rev != reverb_active) {
        if (rev) {
            opendspReverbConfig cfg = {
                .struct_size = sizeof(cfg),
                .room_size = 0.65f, .decay_time = 1.6f, .damping = 0.45f,
                .diffusion = 0.7f, .width = 0.9f, .level = 0.6f
            };
            Result r = opendspReverbSet(0, &cfg);
            if (R_FAILED(r)) {
                Con_Printf("OpenDSP reverb error %08lx\n", (unsigned long)r);
                rev = 0;
            }
        } else opendspReverbSet(0, NULL);
        reverb_active = rev;
    }
    if (echo != echo_active) {
        if (echo) {
            opendspDelayConfig cfg = {
                .struct_size = sizeof(cfg),
                .delay_ms = 275.0f, .feedback = 0.32f,
                .damping = 0.45f, .level = 0.5f
            };
            Result r = opendspDelaySet(1, &cfg);
            if (R_FAILED(r)) {
                Con_Printf("OpenDSP echo error %08lx\n", (unsigned long)r);
                echo = 0;
            }
        } else opendspDelaySet(1, NULL);
        echo_active = echo;
    }
    mix[0] = mix[1] = 1.0f;
    if (rev) {
        mix[4] = mix[5] = 0.35f;
        ndspAuxSetVolume(0, 0.4f);
    } else ndspAuxSetVolume(0, 0);
    if (echo) {
        mix[8] = mix[9] = 0.25f;
        ndspAuxSetVolume(1, 0.3f);
    } else ndspAuxSetVolume(1, 0);
    ndspChnSetMix(0x08, mix);
}

static void flush_new_samples(void)
{
    unsigned total = shm->samples / shm->channels;
    unsigned frames = (unsigned)paintedtime - last_submitted;
    unsigned first, length, bytes;
    if (frames > total) frames = total;
    if (!frames) return;
    first = last_submitted % total;
    length = total - first;
    if (length > frames) length = frames;
    bytes = shm->channels * sizeof(s16);
    if (isDSP) {
        DSP_FlushDataCache((u8*)audiobuffer + first * bytes, length * bytes);
        if (frames > length)
            DSP_FlushDataCache(audiobuffer, (frames - length) * bytes);
    } else {
        CSND_FlushDataCache((u8*)audiobuffer + first * bytes, length * bytes);
        if (frames > length)
            CSND_FlushDataCache(audiobuffer, (frames - length) * bytes);
    }
    last_submitted = paintedtime;
}

qboolean SNDDMA_Init(void)
{
  snd_initialized = 0;
  isDSP = false;
  capabilities = 0;
  reverb_active = echo_active = -1;
  if (!effects_registered) {
    Cvar_RegisterVariable(&h2_reverb);
    Cvar_RegisterVariable(&h2_echo);
    effects_registered = 1;
  }
  if(opendspInit() != 0){
    Con_Printf("dsp::DSP unavailable, trying with csnd:SND...\n");
	isDSP = false;
	if(csndInit() != 0){
		Con_Printf("csnd:SND unavailable, audio off...\n");
		return 0;
	}
  } else {
    isDSP = true;
    opendspGetCapabilities(&capabilities);
  }

  /* Stereo frames -> 4 bytes; mono frames -> 2 bytes. */
  if (isDSP) audiobuffer = linearAlloc(DSP_BUFSIZE * 4);
  else audiobuffer = linearAlloc(CSND_BUFSIZE * 2);
  if (!audiobuffer) {
    if (isDSP) opendspExit(); else csndExit();
    return 0;
  }
  memset(audiobuffer, 0, isDSP ? DSP_BUFSIZE * 4 : CSND_BUFSIZE * 2);

	/* Fill the audio DMA information block */
	shm = &sn;
	shm->splitbuffer = 0;
	shm->samplebits = 16;
	if (isDSP){
		shm->speed = QUAKE_SAMPLERATE;
		shm->channels = 2;
		shm->samples = DSP_BUFSIZE << 1;
	}else{
		shm->speed = QUAKE_SAMPLERATE;
		SAMPLERATE = QUAKE_SAMPLERATE;
		shm->channels = 1;
		shm->samples = CSND_BUFSIZE;	
	}
	shm->samplepos = 0;
	shm->submission_chunk = 1;
	shm->buffer = audiobuffer;
	if (isDSP){
        DSP_FlushDataCache(audiobuffer, DSP_BUFSIZE * 4);
		ndspSetOutputMode(NDSP_OUTPUT_STEREO);
		ndspChnReset(0x08);
		ndspChnWaveBufClear(0x08);
		ndspChnSetInterp(0x08, NDSP_INTERP_LINEAR);
		ndspChnSetRate(0x08, (float)QUAKE_SAMPLERATE);
		ndspChnSetFormat(0x08, NDSP_FORMAT_STEREO_PCM16);
		waveBuf = (ndspWaveBuf*)calloc(1, sizeof(ndspWaveBuf));
        if (!waveBuf) {
            opendspExit();
            linearFree(audiobuffer);
            audiobuffer = NULL;
            shm = NULL;
            return 0;
        }
		createDspBlock(waveBuf, 4, DSP_BUFSIZE * 4, 1, audiobuffer);
		update_effects();
		ndspChnWaveBufAdd(0x08, waveBuf);
	}else {
        Result r = csndPlaySound(0x08, SOUND_LINEAR_INTERP | SOUND_REPEAT | SOUND_FORMAT_16BIT,
                                 QUAKE_SAMPLERATE, 1.0, 0.0,
                                 (u32*)audiobuffer, (u32*)audiobuffer, CSND_BUFSIZE * 2);
        if (R_FAILED(r)) {
            csndExit();
            linearFree(audiobuffer);
            audiobuffer = NULL;
            shm = NULL;
            return 0;
        }
    }
	
  SND_ResetSoundClock();
  last_submitted = 0;
  initial_tick = svcGetSystemTick();

  snd_initialized = 1;
  return 1;
}

int SNDDMA_GetDMAPos(void)
{
  if(!snd_initialized)
    return 0;

  u32 samplepos;
  if (isDSP) {
    /* ndsp reports stereo PCM frames; Quake expects a mono-sample ring index. */
    samplepos = (ndspChnGetSamplePos(0x08) % DSP_BUFSIZE) * 2;
  } else {
    u64 delta = svcGetSystemTick() - initial_tick;
    u64 frames = delta * QUAKE_SAMPLERATE / SYSCLOCK_ARM11;
    samplepos = (u32)(frames % CSND_BUFSIZE);
  }
  shm->samplepos = samplepos;
  return samplepos;
}

void SNDDMA_Shutdown(void)
{
  if(snd_initialized){
	if (isDSP){
		ndspChnWaveBufClear(0x08);
        opendspReverbSet(0, NULL);
        opendspDelaySet(1, NULL);
		opendspExit();
		free(waveBuf);
        waveBuf = NULL;
	}else{
		CSND_SetPlayState(0x08, 0);
        csndExecCmds(true);
		csndExit();
	}
	linearFree(audiobuffer);
  }
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{
	if (snd_initialized){
		flush_new_samples();
        if (isDSP && ((h2_reverb.value > 0) != (reverb_active > 0) ||
                      (h2_echo.value > 0) != (echo_active > 0)))
            update_effects();
	}
}
