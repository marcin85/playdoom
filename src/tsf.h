/* TinySoundFont - v0.9 - SoundFont2 synthesizer - https://github.com/schellingb/TinySoundFont
                                     no warranty implied; use at your own risk
   Do this:
      #define TSF_IMPLEMENTATION
   before you include this file in *one* C or C++ file to create the implementation.
   // i.e. it should look like this:
   #include ...
   #include ...
   #define TSF_IMPLEMENTATION
   #include "tsf.h"

   [OPTIONAL] #define TSF_NO_STDIO to remove stdio dependency
   [OPTIONAL] #define TSF_MALLOC, TSF_REALLOC, and TSF_FREE to avoid stdlib.h
   [OPTIONAL] #define TSF_MEMCPY, TSF_MEMSET to avoid string.h
   [OPTIONAL] #define TSF_POW, TSF_POWF, TSF_EXPF, TSF_LOG, TSF_TAN, TSF_LOG10, TSF_SQRT to avoid math.h

   NOT YET IMPLEMENTED
     - Support for ChorusEffectsSend and ReverbEffectsSend generators
     - Better low-pass filter without lowering performance too much
     - Support for modulators

   LICENSE (MIT)

   Copyright (C) 2017-2023 Bernhard Schelling
   Based on SFZero, Copyright (C) 2012 Steve Folta (https://github.com/stevefolta/SFZero)

   Permission is hereby granted, free of charge, to any person obtaining a copy of this
   software and associated documentation files (the "Software"), to deal in the Software
   without restriction, including without limitation the rights to use, copy, modify, merge,
   publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
   to whom the Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
   PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
   LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef TSF_INCLUDE_TSF_INL
#define TSF_INCLUDE_TSF_INL

#ifdef __cplusplus
extern "C" {
#  define CPP_DEFAULT0 = 0
#else
#  define CPP_DEFAULT0
#endif

//define this if you want the API functions to be static
#ifdef TSF_STATIC
#define TSFDEF static
#else
#define TSFDEF extern
#endif

#define TSF_SAMPLERATE 11025

// The load functions will return a pointer to a struct tsf which all functions
// thereafter take as the first parameter.
// On error the tsf_load* functions will return NULL most likely due to invalid
// data (or if the file did not exist in tsf_load_filename).
typedef struct tsf tsf;

#ifndef TSF_NO_STDIO
// Directly load a SoundFont from a .sf2 file path
TSFDEF tsf* tsf_load_filename(const char* filename);
#endif

// Load a SoundFont from a block of memory
TSFDEF tsf* tsf_load_memory(const void* buffer, int size);

// Stream structure for the generic loading
struct tsf_stream
{
	// Custom data given to the functions as the first parameter
	void* data;

	// Function pointer will be called to read 'size' bytes into ptr (returns number of read bytes)
	int (*read)(void* data, void* ptr, unsigned int size);

	// Function pointer will be called to skip ahead over 'count' bytes (returns 1 on success, 0 on error)
	int (*skip)(void* data, unsigned int count);
};

// Generic SoundFont loading method using the stream structure above
TSFDEF tsf* tsf_load(struct tsf_stream* stream);

// Copy a tsf instance from an existing one, use tsf_close to close it as well.
// All copied tsf instances and their original instance are linked, and share the underlying soundfont.
// This allows loading a soundfont only once, but using it for multiple independent playbacks.
// (This function isn't thread-safe without locking.)
TSFDEF tsf* tsf_copy(tsf* f);

// Free the memory related to this tsf instance
TSFDEF void tsf_close(tsf* f);

// Stop all playing notes immediately and reset all channel parameters
TSFDEF void tsf_reset(tsf* f);

// Returns the preset index from a bank and preset number, or -1 if it does not exist in the loaded SoundFont
TSFDEF int tsf_get_presetindex(const tsf* f, int bank, int preset_number);

// Returns the number of presets in the loaded SoundFont
TSFDEF int tsf_get_presetcount(const tsf* f);

// Returns the name of a preset index >= 0 and < tsf_get_presetcount()
TSFDEF const char* tsf_get_presetname(const tsf* f, int preset_index);

// Returns the name of a preset by bank and preset number
TSFDEF const char* tsf_bank_get_presetname(const tsf* f, int bank, int preset_number);

// Set the global gain as a volume factor
//   global_gain: the desired volume where 1.0 is 100%
TSFDEF void tsf_set_volume(tsf* f, float global_gain);

// Set the maximum number of voices to play simultaneously
// Depending on the soundfond, one note can cause many new voices to be started,
// so don't keep this number too low or otherwise sounds may not play.
//   max_voices: maximum number to pre-allocate and set the limit to
//   (tsf_set_max_voices returns 0 if allocation failed, otherwise 1)
TSFDEF int tsf_set_max_voices(tsf* f, int max_voices);

// Start playing a note
//   preset_index: preset index >= 0 and < tsf_get_presetcount()
//   key: note value between 0 and 127 (60 being middle C)
//   vel: velocity as a float between 0.0 (equal to note off) and 1.0 (full)
//   bank: instrument bank number (alternative to preset_index)
//   preset_number: preset number (alternative to preset_index)
//   (tsf_note_on returns 0 if the allocation of a new voice failed, otherwise 1)
//   (tsf_bank_note_on returns 0 if preset does not exist or allocation failed, otherwise 1)
TSFDEF int tsf_note_on(tsf* f, int preset_index, int key, float vel);
TSFDEF int tsf_bank_note_on(tsf* f, int bank, int preset_number, int key, float vel);

// Stop playing a note
//   (bank_note_off returns 0 if preset does not exist, otherwise 1)
TSFDEF void tsf_note_off(tsf* f, int preset_index, int key);
TSFDEF int  tsf_bank_note_off(tsf* f, int bank, int preset_number, int key);

// Stop playing all notes (end with sustain and release)
TSFDEF void tsf_note_off_all(tsf* f);

// Returns the number of active voices
TSFDEF int tsf_active_voice_count(tsf* f);

// Render output samples into a buffer
// You can either render as signed 16-bit values (tsf_render_short) or
// as 32-bit float values (tsf_render_float)
//   buffer: target buffer of size samples * output_channels * sizeof(type)
TSFDEF void tsf_render_short(tsf* f, short* buffer);

// Higher level channel based functions, set up channel parameters
//   channel: channel number
//   preset_index: preset index >= 0 and < tsf_get_presetcount()
//   preset_number: preset number (alternative to preset_index)
//   flag_mididrums: 0 for normal channels, otherwise apply MIDI drum channel rules
//   bank: instrument bank number (alternative to preset_index)
//   pan: stereo panning value from 0.0 (left) to 1.0 (right) (default 0.5 center)
//   volume: linear volume scale factor (default 1.0 full)
//   pitch_wheel: pitch wheel position 0 to 16383 (default 8192 unpitched)
//   pitch_range: range of the pitch wheel in semitones (default 2.0, total +/- 2 semitones)
//   tuning: tuning of all playing voices in semitones (default 0.0, standard (A440) tuning)
//   (tsf_set_preset_number and set_bank_preset return 0 if preset does not exist, otherwise 1)
//   (tsf_channel_set_... return 0 if a new channel needed allocation and that failed, otherwise 1)
TSFDEF int tsf_channel_set_presetindex(tsf* f, int channel, int preset_index);
TSFDEF int tsf_channel_set_presetnumber(tsf* f, int channel, int preset_number, int flag_mididrums CPP_DEFAULT0);
TSFDEF int tsf_channel_set_bank(tsf* f, int channel, int bank);
TSFDEF int tsf_channel_set_bank_preset(tsf* f, int channel, int bank, int preset_number);
TSFDEF int tsf_channel_set_pan(tsf* f, int channel, float pan);
TSFDEF int tsf_channel_set_volume(tsf* f, int channel, float volume);
TSFDEF int tsf_channel_set_pitchwheel(tsf* f, int channel, int pitch_wheel);
TSFDEF int tsf_channel_set_pitchrange(tsf* f, int channel, float pitch_range);
TSFDEF int tsf_channel_set_tuning(tsf* f, int channel, float tuning);

// Start or stop playing notes on a channel (needs channel preset to be set)
//   channel: channel number
//   key: note value between 0 and 127 (60 being middle C)
//   vel: velocity as a float between 0.0 (equal to note off) and 1.0 (full)
//   (tsf_channel_note_on returns 0 on allocation failure of new voice, otherwise 1)
TSFDEF int tsf_channel_note_on(tsf* f, int channel, int key, float vel);
TSFDEF void tsf_channel_note_off(tsf* f, int channel, int key);
TSFDEF void tsf_channel_note_off_all(tsf* f, int channel); //end with sustain and release
TSFDEF void tsf_channel_sounds_off_all(tsf* f, int channel); //end immediately

// Apply a MIDI control change to the channel (not all controllers are supported!)
//    (tsf_channel_midi_control returns 0 on allocation failure of new channel, otherwise 1)
TSFDEF int tsf_channel_midi_control(tsf* f, int channel, int controller, int control_value);

// Get current values set on the channels
TSFDEF int tsf_channel_get_preset_index(tsf* f, int channel);
TSFDEF int tsf_channel_get_preset_bank(tsf* f, int channel);
TSFDEF int tsf_channel_get_preset_number(tsf* f, int channel);
TSFDEF float tsf_channel_get_pan(tsf* f, int channel);
TSFDEF float tsf_channel_get_volume(tsf* f, int channel);
TSFDEF int tsf_channel_get_pitchwheel(tsf* f, int channel);
TSFDEF float tsf_channel_get_pitchrange(tsf* f, int channel);
TSFDEF float tsf_channel_get_tuning(tsf* f, int channel);

#ifdef __cplusplus
#  undef CPP_DEFAULT0
}
#endif

// end header
// ---------------------------------------------------------------------------------------------------------
#endif //TSF_INCLUDE_TSF_INL

#ifdef TSF_IMPLEMENTATION
#undef TSF_IMPLEMENTATION

// The lower this block size is the more accurate the effects are.
// Increasing the value significantly lowers the CPU usage of the voice rendering.
#ifndef TSF_RENDER_EFFECTSAMPLEBLOCK
#define TSF_RENDER_EFFECTSAMPLEBLOCK 128
#endif

// Grace release time for quick voice off (avoid clicking noise)
#define TSF_FASTRELEASETIME 0.01f

#if !defined(TSF_MALLOC) || !defined(TSF_FREE) || !defined(TSF_REALLOC)
#  include <stdlib.h>
#  define TSF_MALLOC  malloc
#  define TSF_FREE    free
#  define TSF_REALLOC realloc
#endif

#if !defined(TSF_MEMCPY) || !defined(TSF_MEMSET)
#  include <string.h>
#  define TSF_MEMCPY  memcpy
#  define TSF_MEMSET  memset
#endif

#if !defined(TSF_POW) || !defined(TSF_POWF) || !defined(TSF_EXPF) || !defined(TSF_LOG) || !defined(TSF_TAN) || !defined(TSF_LOG10) || !defined(TSF_SQRT)
#  include <math.h>
#  if !defined(__cplusplus) && !defined(NAN) && !defined(powf) && !defined(expf) && !defined(sqrtf)
#    define powf (float)pow // deal with old math.h
#    define expf (float)exp // files that come without
#    define sqrtf (float)sqrt // powf, expf and sqrtf
#  endif
#  define TSF_POWF    powf
#  define TSF_EXPF    expf
#  define TSF_LOG     logf
#  define TSF_TAN     tanf
#  define TSF_LOG10   log10f
#  define TSF_SQRTF   sqrtf
#endif

#ifndef TSF_NO_STDIO
#  include "my_stdio.h"
#endif

#define TSF_TRUE 1
#define TSF_FALSE 0
#define TSF_BOOL char
#define TSF_PI 3.14159265358979323846264338327950288f
#define TSF_NULL 0

#ifdef __cplusplus
extern "C" {
#endif

typedef char tsf_fourcc[4];
typedef signed char tsf_s8;
typedef unsigned char tsf_u8;
typedef unsigned short tsf_u16;
typedef signed short tsf_s16;
typedef unsigned int tsf_u32;
typedef char tsf_char20[20];

#define TSF_FourCCEquals(value1, value2) (value1[0] == value2[0] && value1[1] == value2[1] && value1[2] == value2[2] && value1[3] == value2[3])

struct tsf
{
	struct tsf_preset* presets;
	short* fontSamples;
	struct tsf_voice* voices;
	struct tsf_channels* channels;

	int presetNum;
	int voiceNum;
	int maxVoiceNum;
	unsigned int voicePlayIndex;

	float globalGainDB;
	int* refCount;
};

#ifndef TSF_NO_STDIO
static int tsf_stream_stdio_read(FILE* f, void* ptr, unsigned int size) { return (int)fread(ptr, 1, size, f); }
static int tsf_stream_stdio_skip(FILE* f, unsigned int count) { return !fseek(f, count, SEEK_CUR); }
TSFDEF tsf* tsf_load_filename(const char* filename)
{
	tsf* res;
	struct tsf_stream stream = { TSF_NULL, (int(*)(void*,void*,unsigned int))&tsf_stream_stdio_read, (int(*)(void*,unsigned int))&tsf_stream_stdio_skip };
	#if __STDC_WANT_SECURE_LIB__
	FILE* f = TSF_NULL; fopen_s(&f, filename, "rb");
	#else
	FILE* f = fopen(filename, "rb");
	#endif
	if (!f)
	{
		//if (e) *e = TSF_FILENOTFOUND;
		return TSF_NULL;
	}
	stream.data = f;
	res = tsf_load(&stream);
	fclose(f);
	return res;
}
#endif

struct tsf_stream_memory { const char* buffer; unsigned int total, pos; };
static int tsf_stream_memory_read(struct tsf_stream_memory* m, void* ptr, unsigned int size) { if (size > m->total - m->pos) size = m->total - m->pos; TSF_MEMCPY(ptr, m->buffer+m->pos, size); m->pos += size; return size; }
static int tsf_stream_memory_skip(struct tsf_stream_memory* m, unsigned int count) { if (m->pos + count > m->total) return 0; m->pos += count; return 1; }
TSFDEF tsf* tsf_load_memory(const void* buffer, int size)
{
	struct tsf_stream stream = { TSF_NULL, (int(*)(void*,void*,unsigned int))&tsf_stream_memory_read, (int(*)(void*,unsigned int))&tsf_stream_memory_skip };
	struct tsf_stream_memory f = { 0, 0, 0 };
	f.buffer = (const char*)buffer;
	f.total = size;
	stream.data = &f;
	return tsf_load(&stream);
}

enum { TSF_LOOPMODE_NONE, TSF_LOOPMODE_CONTINUOUS, TSF_LOOPMODE_SUSTAIN };

enum { TSF_SEGMENT_NONE, TSF_SEGMENT_DELAY, TSF_SEGMENT_ATTACK, TSF_SEGMENT_HOLD, TSF_SEGMENT_DECAY, TSF_SEGMENT_SUSTAIN, TSF_SEGMENT_RELEASE, TSF_SEGMENT_DONE };

struct tsf_hydra
{
	struct tsf_hydra_phdr *phdrs; struct tsf_hydra_pbag *pbags; struct tsf_hydra_pmod *pmods;
	struct tsf_hydra_pgen *pgens; struct tsf_hydra_inst *insts; struct tsf_hydra_ibag *ibags;
	struct tsf_hydra_imod *imods; struct tsf_hydra_igen *igens; struct tsf_hydra_shdr *shdrs;
	int phdrNum, pbagNum, pmodNum, pgenNum, instNum, ibagNum, imodNum, igenNum, shdrNum;
};

union tsf_hydra_genamount { struct { tsf_u8 lo, hi; } range; tsf_s16 shortAmount; tsf_u16 wordAmount; };
struct tsf_hydra_phdr { tsf_char20 presetName; tsf_u16 preset, bank, presetBagNdx; tsf_u32 library, genre, morphology; };
struct tsf_hydra_pbag { tsf_u16 genNdx, modNdx; };
struct tsf_hydra_pmod { tsf_u16 modSrcOper, modDestOper; tsf_s16 modAmount; tsf_u16 modAmtSrcOper, modTransOper; };
struct tsf_hydra_pgen { tsf_u16 genOper; union tsf_hydra_genamount genAmount; };
struct tsf_hydra_inst { tsf_char20 instName; tsf_u16 instBagNdx; };
struct tsf_hydra_ibag { tsf_u16 instGenNdx, instModNdx; };
struct tsf_hydra_imod { tsf_u16 modSrcOper, modDestOper; tsf_s16 modAmount; tsf_u16 modAmtSrcOper, modTransOper; };
struct tsf_hydra_igen { tsf_u16 genOper; union tsf_hydra_genamount genAmount; };
struct tsf_hydra_shdr { tsf_char20 sampleName; tsf_u32 start, end, startLoop, endLoop, sampleRate; tsf_u8 originalPitch; tsf_s8 pitchCorrection; tsf_u16 sampleLink, sampleType; };

#define TSFR(FIELD) stream->read(stream->data, &i->FIELD, sizeof(i->FIELD));
static void tsf_hydra_read_phdr(struct tsf_hydra_phdr* i, struct tsf_stream* stream) { TSFR(presetName) TSFR(preset) TSFR(bank) TSFR(presetBagNdx) TSFR(library) TSFR(genre) TSFR(morphology) }
static void tsf_hydra_read_pbag(struct tsf_hydra_pbag* i, struct tsf_stream* stream) { TSFR(genNdx) TSFR(modNdx) }
static void tsf_hydra_read_pmod(struct tsf_hydra_pmod* i, struct tsf_stream* stream) { TSFR(modSrcOper) TSFR(modDestOper) TSFR(modAmount) TSFR(modAmtSrcOper) TSFR(modTransOper) }
static void tsf_hydra_read_pgen(struct tsf_hydra_pgen* i, struct tsf_stream* stream) { TSFR(genOper) TSFR(genAmount) }
static void tsf_hydra_read_inst(struct tsf_hydra_inst* i, struct tsf_stream* stream) { TSFR(instName) TSFR(instBagNdx) }
static void tsf_hydra_read_ibag(struct tsf_hydra_ibag* i, struct tsf_stream* stream) { TSFR(instGenNdx) TSFR(instModNdx) }
static void tsf_hydra_read_imod(struct tsf_hydra_imod* i, struct tsf_stream* stream) { TSFR(modSrcOper) TSFR(modDestOper) TSFR(modAmount) TSFR(modAmtSrcOper) TSFR(modTransOper) }
static void tsf_hydra_read_igen(struct tsf_hydra_igen* i, struct tsf_stream* stream) { TSFR(genOper) TSFR(genAmount) }
static void tsf_hydra_read_shdr(struct tsf_hydra_shdr* i, struct tsf_stream* stream) { TSFR(sampleName) TSFR(start) TSFR(end) TSFR(startLoop) TSFR(endLoop) TSFR(sampleRate) TSFR(originalPitch) TSFR(pitchCorrection) TSFR(sampleLink) TSFR(sampleType) }
#undef TSFR

struct tsf_riffchunk { tsf_fourcc id; tsf_u32 size; };
struct tsf_envelope { float delay, attack, hold, decay, sustain, release, keynumToHold, keynumToDecay; };
struct tsf_voice_envelope { float level, slope; int samplesUntilNextSegment; short segment, midiVelocity; struct tsf_envelope parameters; TSF_BOOL segmentIsExponential, isAmpEnv; };

struct tsf_region
{
	int loop_mode;
	unsigned int sample_rate;
	unsigned char lokey, hikey, lovel, hivel;
	unsigned int group, offset, end, loop_start, loop_end;
	int transpose, tune, pitch_keycenter, pitch_keytrack;
	float attenuation, pan;
	struct tsf_envelope ampenv;
};

struct tsf_preset
{
	tsf_char20 presetName;
	tsf_u16 preset, bank;
	struct tsf_region* regions;
	int regionNum;
};

struct tsf_voice
{
	int playingPreset, playingKey, playingChannel;
	struct tsf_region* region;
	float  pitchInputTimecents, pitchOutputFactor;
	float  sourceSamplePosition;
	float  noteGainDB, panFactorLeft, panFactorRight;
	unsigned int playIndex, loopStart, loopEnd;
	struct tsf_voice_envelope ampenv;
};

struct tsf_channel
{
	unsigned short presetIndex, bank, pitchWheel, midiPan, midiVolume, midiExpression, midiRPN, midiData;
	float panOffset, gainDB, pitchRange, tuning;
};

struct tsf_channels
{
	void (*setupVoice)(tsf* f, struct tsf_voice* voice);
	int channelNum, activeChannel;
	struct tsf_channel channels[1];
};

static float tsf_timecents2Secsf(float timecents) { return TSF_POWF(2.0f, timecents / 1200.0f); }
static float tsf_cents2Hertz(float cents) { return 8.176f * TSF_POWF(2.0f, cents / 1200.0f); }
static float tsf_decibelsToGain(float db) { return (db > -100.f ? TSF_POWF(10.0f, db * 0.05f) : 0); }
static float tsf_gainToDecibels(float gain) { return (gain <= .00001f ? -100.f : (float)(20.0f * TSF_LOG10(gain))); }

static TSF_BOOL tsf_riffchunk_read(struct tsf_riffchunk* parent, struct tsf_riffchunk* chunk, struct tsf_stream* stream)
{
	TSF_BOOL IsRiff, IsList;
	if (parent && sizeof(tsf_fourcc) + sizeof(tsf_u32) > parent->size) return TSF_FALSE;
	if (!stream->read(stream->data, &chunk->id, sizeof(tsf_fourcc)) || *chunk->id <= ' ' || *chunk->id >= 'z') return TSF_FALSE;
	if (!stream->read(stream->data, &chunk->size, sizeof(tsf_u32))) return TSF_FALSE;
	if (parent && sizeof(tsf_fourcc) + sizeof(tsf_u32) + chunk->size > parent->size) return TSF_FALSE;
	if (parent) parent->size -= sizeof(tsf_fourcc) + sizeof(tsf_u32) + chunk->size;
	IsRiff = TSF_FourCCEquals(chunk->id, "RIFF"), IsList = TSF_FourCCEquals(chunk->id, "LIST");
	if (IsRiff && parent) return TSF_FALSE; //not allowed
	if (!IsRiff && !IsList) return TSF_TRUE; //custom type without sub type
	if (!stream->read(stream->data, &chunk->id, sizeof(tsf_fourcc)) || *chunk->id <= ' ' || *chunk->id >= 'z') return TSF_FALSE;
	chunk->size -= sizeof(tsf_fourcc);
	return TSF_TRUE;
}

static void tsf_region_clear(struct tsf_region* i, TSF_BOOL for_relative)
{
	TSF_MEMSET(i, 0, sizeof(struct tsf_region));
	i->hikey = i->hivel = 127;
	i->pitch_keycenter = 60; // C4
	if (for_relative) return;

	i->pitch_keytrack = 100;

	i->pitch_keycenter = -1;

	// SF2 defaults in timecents.
	i->ampenv.delay = i->ampenv.attack = i->ampenv.hold = i->ampenv.decay = i->ampenv.release = -12000.0f;
}

static void tsf_region_operator(struct tsf_region* region, tsf_u16 genOper, union tsf_hydra_genamount* amount, struct tsf_region* merge_region)
{
	enum
	{
		_GEN_TYPE_MASK       = 0x0F,
		GEN_FLOAT            = 0x01,
		GEN_INT              = 0x02,
		GEN_UINT_ADD         = 0x03,
		GEN_UINT_ADD15       = 0x04,
		GEN_KEYRANGE         = 0x05,
		GEN_VELRANGE         = 0x06,
		GEN_LOOPMODE         = 0x07,
		GEN_GROUP            = 0x08,
		GEN_KEYCENTER        = 0x09,

		_GEN_LIMIT_MASK      = 0xF0,
		GEN_INT_LIMIT12K     = 0x10, //min -12000, max 12000
		GEN_INT_LIMITFC      = 0x20, //min 1500, max 13500
		GEN_INT_LIMITQ       = 0x30, //min 0, max 960
		GEN_INT_LIMIT960     = 0x40, //min -960, max 960
		GEN_INT_LIMIT16K4500 = 0x50, //min -16000, max 4500
		GEN_FLOAT_LIMIT12K5K = 0x60, //min -12000, max 5000
		GEN_FLOAT_LIMIT12K8K = 0x70, //min -12000, max 8000
		GEN_FLOAT_LIMIT1200  = 0x80, //min -1200, max 1200
		GEN_FLOAT_LIMITPAN   = 0x90, //* .001f, min -.5f, max .5f,
		GEN_FLOAT_LIMITATTN  = 0xA0, //* .1f, min 0, max 144.0
		GEN_FLOAT_MAX1000    = 0xB0, //min 0, max 1000
		GEN_FLOAT_MAX1440    = 0xC0, //min 0, max 1440

		_GEN_MAX = 59
	};
	#define _TSFREGIONOFFSET(TYPE, FIELD) (unsigned char)(((TYPE*)&((struct tsf_region*)0)->FIELD) - (TYPE*)0)
	#define _TSFREGIONENVOFFSET(TYPE, ENV, FIELD) (unsigned char)(((TYPE*)&((&(((struct tsf_region*)0)->ENV))->FIELD)) - (TYPE*)0)
	static const struct { unsigned char mode, offset; } genMetas[_GEN_MAX] =
	{
		{ GEN_UINT_ADD                     , _TSFREGIONOFFSET(unsigned int, offset               ) }, // 0 StartAddrsOffset
		{ GEN_UINT_ADD                     , _TSFREGIONOFFSET(unsigned int, end                  ) }, // 1 EndAddrsOffset
		{ GEN_UINT_ADD                     , _TSFREGIONOFFSET(unsigned int, loop_start           ) }, // 2 StartloopAddrsOffset
		{ GEN_UINT_ADD                     , _TSFREGIONOFFSET(unsigned int, loop_end             ) }, // 3 EndloopAddrsOffset
		{ GEN_UINT_ADD15                   , _TSFREGIONOFFSET(unsigned int, offset               ) }, // 4 StartAddrsCoarseOffset
		{ 0                                , (0                                                  ) }, // 5 ModLfoToPitch
		{ 0                                , (0                                                  ) }, // 6 VibLfoToPitch
		{ 0                                , (0                                                  ) }, // 7 ModEnvToPitch
		{ 0                                , (0                                                  ) }, // 8 InitialFilterFc
		{ 0                                , (0                                                  ) }, // 9 InitialFilterQ
		{ 0                                , (0                                                  ) }, //10 ModLfoToFilterFc
		{ 0                                , (0                                                  ) }, //11 ModEnvToFilterFc
		{ GEN_UINT_ADD15                   , _TSFREGIONOFFSET(unsigned int, end                  ) }, //12 EndAddrsCoarseOffset
		{ 0                                , (0                                                  ) }, //13 ModLfoToVolume
		{ 0                                , (0                                                  ) }, //   Unused
		{ 0                                , (0                                                  ) }, //15 ChorusEffectsSend (unsupported)
		{ 0                                , (0                                                  ) }, //16 ReverbEffectsSend (unsupported)
		{ GEN_FLOAT | GEN_FLOAT_LIMITPAN   , _TSFREGIONOFFSET(       float, pan                  ) }, //17 Pan
		{ 0                                , (0                                                  ) }, //   Unused
		{ 0                                , (0                                                  ) }, //   Unused
		{ 0                                , (0                                                  ) }, //   Unused
		{ 0                                , (0                                                  ) }, //21 DelayModLFO
		{ 0                                , (0                                                  ) }, //22 FreqModLFO
		{ 0                                , (0                                                  ) }, //23 DelayVibLFO
		{ 0                                , (0                                                  ) }, //24 FreqVibLFO
		{ 0                                , (0                                                  ) }, //25 DelayModEnv
		{ 0                                , (0                                                  ) }, //26 AttackModEnv
		{ 0                                , (0                                                  ) }, //27 HoldModEnv
		{ 0                                , (0                                                  ) }, //28 DecayModEnv
		{ 0                                , (0                                                  ) }, //29 SustainModEnv
		{ 0                                , (0                                                  ) }, //30 ReleaseModEnv
		{ 0                                , (0                                                  ) }, //31 KeynumToModEnvHold
		{ 0                                , (0                                                  ) }, //32 KeynumToModEnvDecay
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K5K , _TSFREGIONENVOFFSET(    float, ampenv, delay        ) }, //33 DelayVolEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K8K , _TSFREGIONENVOFFSET(    float, ampenv, attack       ) }, //34 AttackVolEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K5K , _TSFREGIONENVOFFSET(    float, ampenv, hold         ) }, //35 HoldVolEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K8K , _TSFREGIONENVOFFSET(    float, ampenv, decay        ) }, //36 DecayVolEnv
		{ GEN_FLOAT | GEN_FLOAT_MAX1440    , _TSFREGIONENVOFFSET(    float, ampenv, sustain      ) }, //37 SustainVolEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K8K , _TSFREGIONENVOFFSET(    float, ampenv, release      ) }, //38 ReleaseVolEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT1200  , _TSFREGIONENVOFFSET(    float, ampenv, keynumToHold ) }, //39 KeynumToVolEnvHold
		{ GEN_FLOAT | GEN_FLOAT_LIMIT1200  , _TSFREGIONENVOFFSET(    float, ampenv, keynumToDecay) }, //40 KeynumToVolEnvDecay
		{ 0                                , (0                                                  ) }, //   Instrument (special)
		{ 0                                , (0                                                  ) }, //   Reserved
		{ GEN_KEYRANGE                     , (0                                                  ) }, //43 KeyRange
		{ GEN_VELRANGE                     , (0                                                  ) }, //44 VelRange
		{ GEN_UINT_ADD15                   , _TSFREGIONOFFSET(unsigned int, loop_start           ) }, //45 StartloopAddrsCoarseOffset
		{ 0                                , (0                                                  ) }, //46 Keynum (special)
		{ 0                                , (0                                                  ) }, //47 Velocity (special)
		{ GEN_FLOAT | GEN_FLOAT_LIMITATTN  , _TSFREGIONOFFSET(       float, attenuation          ) }, //48 InitialAttenuation
		{ 0                                , (0                                                  ) }, //   Reserved
		{ GEN_UINT_ADD15                   , _TSFREGIONOFFSET(unsigned int, loop_end             ) }, //50 EndloopAddrsCoarseOffset
		{ GEN_INT                          , _TSFREGIONOFFSET(         int, transpose            ) }, //51 CoarseTune
		{ GEN_INT                          , _TSFREGIONOFFSET(         int, tune                 ) }, //52 FineTune
		{ 0                                , (0                                                  ) }, //   SampleID (special)
		{ GEN_LOOPMODE                     , _TSFREGIONOFFSET(         int, loop_mode            ) }, //54 SampleModes
		{ 0                                , (0                                                  ) }, //   Reserved
		{ GEN_INT                          , _TSFREGIONOFFSET(         int, pitch_keytrack       ) }, //56 ScaleTuning
		{ GEN_GROUP                        , _TSFREGIONOFFSET(unsigned int, group                ) }, //57 ExclusiveClass
		{ GEN_KEYCENTER                    , _TSFREGIONOFFSET(         int, pitch_keycenter      ) }, //58 OverridingRootKey
	};
	#undef _TSFREGIONOFFSET
	#undef _TSFREGIONENVOFFSET
	if (amount)
	{
		int offset;
		if (genOper >= _GEN_MAX) return;
		offset = genMetas[genOper].offset;
		switch (genMetas[genOper].mode & _GEN_TYPE_MASK)
		{
			case GEN_FLOAT:      ((       float*)region)[offset]  = amount->shortAmount;     return;
			case GEN_INT:        ((         int*)region)[offset]  = amount->shortAmount;     return;
			case GEN_UINT_ADD:   ((unsigned int*)region)[offset] += amount->shortAmount;     return;
			case GEN_UINT_ADD15: ((unsigned int*)region)[offset] += amount->shortAmount<<15; return;
			case GEN_KEYRANGE:   region->lokey = amount->range.lo; region->hikey = amount->range.hi; return;
			case GEN_VELRANGE:   region->lovel = amount->range.lo; region->hivel = amount->range.hi; return;
			case GEN_LOOPMODE:   region->loop_mode       = ((amount->wordAmount&3) == 3 ? TSF_LOOPMODE_SUSTAIN : ((amount->wordAmount&3) == 1 ? TSF_LOOPMODE_CONTINUOUS : TSF_LOOPMODE_NONE)); return;
			case GEN_GROUP:      region->group           = amount->wordAmount;  return;
			case GEN_KEYCENTER:  region->pitch_keycenter = amount->shortAmount; return;
		}
	}
	else //merge regions and clamp values
	{
		for (genOper = 0; genOper != _GEN_MAX; genOper++)
		{
			int offset = genMetas[genOper].offset;
			switch (genMetas[genOper].mode & _GEN_TYPE_MASK)
			{
				case GEN_FLOAT:
				{
					float *val = &((float*)region)[offset], vfactor, vmin, vmax;
					*val += ((float*)merge_region)[offset];
					switch (genMetas[genOper].mode & _GEN_LIMIT_MASK)
					{
						case GEN_FLOAT_LIMIT12K5K: vfactor =   1.0f; vmin = -12000.0f; vmax = 5000.0f; break;
						case GEN_FLOAT_LIMIT12K8K: vfactor =   1.0f; vmin = -12000.0f; vmax = 8000.0f; break;
						case GEN_FLOAT_LIMIT1200:  vfactor =   1.0f; vmin =  -1200.0f; vmax = 1200.0f; break;
						case GEN_FLOAT_LIMITPAN:   vfactor = 0.001f; vmin =     -0.5f; vmax =    0.5f; break;
						case GEN_FLOAT_LIMITATTN:  vfactor =   0.1f; vmin =      0.0f; vmax =  144.0f; break;
						case GEN_FLOAT_MAX1000:    vfactor =   1.0f; vmin =      0.0f; vmax = 1000.0f; break;
						case GEN_FLOAT_MAX1440:    vfactor =   1.0f; vmin =      0.0f; vmax = 1440.0f; break;
						default: continue;
					}
					*val *= vfactor;
					if      (*val < vmin) *val = vmin;
					else if (*val > vmax) *val = vmax;
					continue;
				}
				case GEN_INT:
				{
					int *val = &((int*)region)[offset], vmin, vmax;
					*val += ((int*)merge_region)[offset];
					switch (genMetas[genOper].mode & _GEN_LIMIT_MASK)
					{
						case GEN_INT_LIMIT12K:     vmin = -12000; vmax = 12000; break;
						case GEN_INT_LIMITFC:      vmin =   1500; vmax = 13500; break;
						case GEN_INT_LIMITQ:       vmin =      0; vmax =   960; break;
						case GEN_INT_LIMIT960:     vmin =   -960; vmax =   960; break;
						case GEN_INT_LIMIT16K4500: vmin = -16000; vmax =  4500; break;
						default: continue;
					}
					if      (*val < vmin) *val = vmin;
					else if (*val > vmax) *val = vmax;
					continue;
				}
				case GEN_UINT_ADD:
				{
					((unsigned int*)region)[offset] += ((unsigned int*)merge_region)[offset];
					continue;
				}
			}
		}
	}
}

static void tsf_region_envtosecs(struct tsf_envelope* p, TSF_BOOL sustainIsGain)
{
	// EG times need to be converted from timecents to seconds.
	// Pin very short EG segments.  Timecents don't get to zero, and our EG is
	// happier with zero values.
	p->delay   = (p->delay   < -11950.0f ? 0.0f : tsf_timecents2Secsf(p->delay));
	p->attack  = (p->attack  < -11950.0f ? 0.0f : tsf_timecents2Secsf(p->attack));
	p->release = (p->release < -11950.0f ? 0.0f : tsf_timecents2Secsf(p->release));

	// If we have dynamic hold or decay times depending on key number we need
	// to keep the values in timecents so we can calculate it during startNote
	if (!p->keynumToHold)  p->hold  = (p->hold  < -11950.0f ? 0.0f : tsf_timecents2Secsf(p->hold));
	if (!p->keynumToDecay) p->decay = (p->decay < -11950.0f ? 0.0f : tsf_timecents2Secsf(p->decay));

	if (p->sustain < 0.0f) p->sustain = 0.0f;
	else if (sustainIsGain) p->sustain = tsf_decibelsToGain(-p->sustain / 10.0f);
	else p->sustain = 1.0f - (p->sustain / 1000.0f);
}

static int tsf_load_presets(tsf* res, struct tsf_hydra *hydra, unsigned int fontSampleCount)
{
	enum { GenInstrument = 41, GenKeyRange = 43, GenVelRange = 44, GenSampleID = 53 };
	// Read each preset.
	struct tsf_hydra_phdr *pphdr, *pphdrMax;
	res->presetNum = hydra->phdrNum - 1;
	res->presets = (struct tsf_preset*)TSF_MALLOC(res->presetNum * sizeof(struct tsf_preset));
	for (int i = 0; i != res->presetNum; i++) res->presets[i].regions = TSF_NULL;
	for (pphdr = hydra->phdrs, pphdrMax = pphdr + hydra->phdrNum - 1; pphdr != pphdrMax; pphdr++)
	{
		int sortedIndex = 0, region_index = 0;
		struct tsf_hydra_phdr *otherphdr;
		struct tsf_preset* preset;
		struct tsf_hydra_pbag *ppbag, *ppbagEnd;
		struct tsf_region globalRegion;
		for (otherphdr = hydra->phdrs; otherphdr != pphdrMax; otherphdr++)
		{
			if (otherphdr == pphdr || otherphdr->bank > pphdr->bank) continue;
			else if (otherphdr->bank < pphdr->bank) sortedIndex++;
			else if (otherphdr->preset > pphdr->preset) continue;
			else if (otherphdr->preset < pphdr->preset) sortedIndex++;
			else if (otherphdr < pphdr) sortedIndex++;
		}

		preset = &res->presets[sortedIndex];
		TSF_MEMCPY(preset->presetName, pphdr->presetName, sizeof(preset->presetName));
		preset->presetName[sizeof(preset->presetName)-1] = '\0'; //should be zero terminated in source file but make sure
		preset->bank = pphdr->bank;
		preset->preset = pphdr->preset;
		preset->regionNum = 0;

		//count regions covered by this preset
		for (ppbag = hydra->pbags + pphdr->presetBagNdx, ppbagEnd = hydra->pbags + pphdr[1].presetBagNdx; ppbag != ppbagEnd; ppbag++)
		{
			unsigned char plokey = 0, phikey = 127, plovel = 0, phivel = 127;
			struct tsf_hydra_pgen *ppgen, *ppgenEnd; struct tsf_hydra_inst *pinst; struct tsf_hydra_ibag *pibag, *pibagEnd; struct tsf_hydra_igen *pigen, *pigenEnd;
			for (ppgen = hydra->pgens + ppbag->genNdx, ppgenEnd = hydra->pgens + ppbag[1].genNdx; ppgen != ppgenEnd; ppgen++)
			{
				if (ppgen->genOper == GenKeyRange) { plokey = ppgen->genAmount.range.lo; phikey = ppgen->genAmount.range.hi; continue; }
				if (ppgen->genOper == GenVelRange) { plovel = ppgen->genAmount.range.lo; phivel = ppgen->genAmount.range.hi; continue; }
				if (ppgen->genOper != GenInstrument) continue;
				if (ppgen->genAmount.wordAmount >= hydra->instNum) continue;
				pinst = hydra->insts + ppgen->genAmount.wordAmount;
				for (pibag = hydra->ibags + pinst->instBagNdx, pibagEnd = hydra->ibags + pinst[1].instBagNdx; pibag != pibagEnd; pibag++)
				{
					unsigned char ilokey = 0, ihikey = 127, ilovel = 0, ihivel = 127;
					for (pigen = hydra->igens + pibag->instGenNdx, pigenEnd = hydra->igens + pibag[1].instGenNdx; pigen != pigenEnd; pigen++)
					{
						if (pigen->genOper == GenKeyRange) { ilokey = pigen->genAmount.range.lo; ihikey = pigen->genAmount.range.hi; continue; }
						if (pigen->genOper == GenVelRange) { ilovel = pigen->genAmount.range.lo; ihivel = pigen->genAmount.range.hi; continue; }
						if (pigen->genOper == GenSampleID && ihikey >= plokey && ilokey <= phikey && ihivel >= plovel && ilovel <= phivel) preset->regionNum++;
					}
				}
			}
		}

		preset->regions = (struct tsf_region*)TSF_MALLOC(preset->regionNum * sizeof(struct tsf_region));
		tsf_region_clear(&globalRegion, TSF_TRUE);

		// Zones.
		for (ppbag = hydra->pbags + pphdr->presetBagNdx, ppbagEnd = hydra->pbags + pphdr[1].presetBagNdx; ppbag != ppbagEnd; ppbag++)
		{
			struct tsf_hydra_pgen *ppgen, *ppgenEnd; struct tsf_hydra_inst *pinst; struct tsf_hydra_ibag *pibag, *pibagEnd; struct tsf_hydra_igen *pigen, *pigenEnd;
			struct tsf_region presetRegion = globalRegion;
			int hadGenInstrument = 0;

			// Generators.
			for (ppgen = hydra->pgens + ppbag->genNdx, ppgenEnd = hydra->pgens + ppbag[1].genNdx; ppgen != ppgenEnd; ppgen++)
			{
				// Instrument.
				if (ppgen->genOper == GenInstrument)
				{
					struct tsf_region instRegion;
					tsf_u16 whichInst = ppgen->genAmount.wordAmount;
					if (whichInst >= hydra->instNum) continue;

					tsf_region_clear(&instRegion, TSF_FALSE);
					pinst = &hydra->insts[whichInst];
					for (pibag = hydra->ibags + pinst->instBagNdx, pibagEnd = hydra->ibags + pinst[1].instBagNdx; pibag != pibagEnd; pibag++)
					{
						// Generators.
						struct tsf_region zoneRegion = instRegion;
						int hadSampleID = 0;
						for (pigen = hydra->igens + pibag->instGenNdx, pigenEnd = hydra->igens + pibag[1].instGenNdx; pigen != pigenEnd; pigen++)
						{
							if (pigen->genOper == GenSampleID)
							{
								struct tsf_hydra_shdr* pshdr;

								//preset region key and vel ranges are a filter for the zone regions
								if (zoneRegion.hikey < presetRegion.lokey || zoneRegion.lokey > presetRegion.hikey) continue;
								if (zoneRegion.hivel < presetRegion.lovel || zoneRegion.lovel > presetRegion.hivel) continue;
								if (presetRegion.lokey > zoneRegion.lokey) zoneRegion.lokey = presetRegion.lokey;
								if (presetRegion.hikey < zoneRegion.hikey) zoneRegion.hikey = presetRegion.hikey;
								if (presetRegion.lovel > zoneRegion.lovel) zoneRegion.lovel = presetRegion.lovel;
								if (presetRegion.hivel < zoneRegion.hivel) zoneRegion.hivel = presetRegion.hivel;

								//sum regions
								tsf_region_operator(&zoneRegion, 0, TSF_NULL, &presetRegion);

								// EG times need to be converted from timecents to seconds.
								tsf_region_envtosecs(&zoneRegion.ampenv, TSF_TRUE);

								// Fixup sample positions
								pshdr = &hydra->shdrs[pigen->genAmount.wordAmount];
								zoneRegion.offset += pshdr->start;
								zoneRegion.end += pshdr->end;
								zoneRegion.loop_start += pshdr->startLoop;
								zoneRegion.loop_end += pshdr->endLoop;
								if (pshdr->endLoop > 0) zoneRegion.loop_end -= 1;
								if (zoneRegion.loop_end > fontSampleCount) zoneRegion.loop_end = fontSampleCount;
								if (zoneRegion.pitch_keycenter == -1) zoneRegion.pitch_keycenter = pshdr->originalPitch;
								zoneRegion.tune += pshdr->pitchCorrection;
								zoneRegion.sample_rate = pshdr->sampleRate;
								if (zoneRegion.end && zoneRegion.end < fontSampleCount) zoneRegion.end++;
								else zoneRegion.end = fontSampleCount;

								preset->regions[region_index] = zoneRegion;
								region_index++;
								hadSampleID = 1;
							}
							else tsf_region_operator(&zoneRegion, pigen->genOper, &pigen->genAmount, TSF_NULL);
						}

						// Handle instrument's global zone.
						if (pibag == hydra->ibags + pinst->instBagNdx && !hadSampleID)
							instRegion = zoneRegion;

						// Modulators (TODO)
						//if (ibag->instModNdx < ibag[1].instModNdx) addUnsupportedOpcode("any modulator");
					}
					hadGenInstrument = 1;
				}
				else tsf_region_operator(&presetRegion, ppgen->genOper, &ppgen->genAmount, TSF_NULL);
			}

			// Modulators (TODO)
			//if (pbag->modNdx < pbag[1].modNdx) addUnsupportedOpcode("any modulator");

			// Handle preset's global zone.
			if (ppbag == hydra->pbags + pphdr->presetBagNdx && !hadGenInstrument)
				globalRegion = presetRegion;
		}
	}
	return 1;
}

static int tsf_load_samples(short** pRawBuffer, unsigned int* pSmplCount, struct tsf_riffchunk *chunkSmpl, struct tsf_stream* stream)
{
	// Inline convert the samples from short to float
	short *res, *out; const short *in;
	*pSmplCount = chunkSmpl->size / (unsigned int)sizeof(short);
	*pRawBuffer = (short*)TSF_MALLOC(*pSmplCount * sizeof(short));
	if (!stream->read(stream->data, *pRawBuffer, chunkSmpl->size)) return 0;
	return 1;
}

static int tsf_voice_envelope_release_samples(struct tsf_voice_envelope* e)
{
	return (int)((e->parameters.release <= 0 ? TSF_FASTRELEASETIME : e->parameters.release) * TSF_SAMPLERATE);
}

static void tsf_voice_envelope_nextsegment(struct tsf_voice_envelope* e, short active_segment)
{
	switch (active_segment)
	{
		case TSF_SEGMENT_NONE:
			e->samplesUntilNextSegment = (int)(e->parameters.delay * TSF_SAMPLERATE);
			if (e->samplesUntilNextSegment > 0)
			{
				e->segment = TSF_SEGMENT_DELAY;
				e->segmentIsExponential = TSF_FALSE;
				e->level = 0.0;
				e->slope = 0.0;
				return;
			}
			/* fall through */
		case TSF_SEGMENT_DELAY:
			e->samplesUntilNextSegment = (int)(e->parameters.attack * TSF_SAMPLERATE);
			if (e->samplesUntilNextSegment > 0)
			{
				if (!e->isAmpEnv)
				{
					//mod env attack duration scales with velocity (velocity of 1 is full duration, max velocity is 0.125 times duration)
					e->samplesUntilNextSegment = (int)(e->parameters.attack * ((145 - e->midiVelocity) / 144.0f) * TSF_SAMPLERATE);
				}
				e->segment = TSF_SEGMENT_ATTACK;
				e->segmentIsExponential = TSF_FALSE;
				e->level = 0.0f;
				e->slope = 1.0f / e->samplesUntilNextSegment;
				return;
			}
			/* fall through */
		case TSF_SEGMENT_ATTACK:
			e->samplesUntilNextSegment = (int)(e->parameters.hold * TSF_SAMPLERATE);
			if (e->samplesUntilNextSegment > 0)
			{
				e->segment = TSF_SEGMENT_HOLD;
				e->segmentIsExponential = TSF_FALSE;
				e->level = 1.0f;
				e->slope = 0.0f;
				return;
			}
			/* fall through */
		case TSF_SEGMENT_HOLD:
			e->samplesUntilNextSegment = (int)(e->parameters.decay * TSF_SAMPLERATE);
			if (e->samplesUntilNextSegment > 0)
			{
				e->segment = TSF_SEGMENT_DECAY;
				e->level = 1.0f;
				if (e->isAmpEnv)
				{
					// I don't truly understand this; just following what LinuxSampler does.
					float mysterySlope = -9.226f / e->samplesUntilNextSegment;
					e->slope = TSF_EXPF(mysterySlope);
					e->segmentIsExponential = TSF_TRUE;
					if (e->parameters.sustain > 0.0f)
					{
						// Again, this is following LinuxSampler's example, which is similar to
						// SF2-style decay, where "decay" specifies the time it would take to
						// get to zero, not to the sustain level.  The SFZ spec is not that
						// specific about what "decay" means, so perhaps it's really supposed
						// to specify the time to reach the sustain level.
						e->samplesUntilNextSegment = (int)(TSF_LOG(e->parameters.sustain) / mysterySlope);
					}
				}
				else
				{
					e->slope = -1.0f / e->samplesUntilNextSegment;
					e->samplesUntilNextSegment = (int)(e->parameters.decay * (1.0f - e->parameters.sustain) * TSF_SAMPLERATE);
					e->segmentIsExponential = TSF_FALSE;
				}
				return;
			}
			/* fall through */
		case TSF_SEGMENT_DECAY:
			e->segment = TSF_SEGMENT_SUSTAIN;
			e->level = e->parameters.sustain;
			e->slope = 0.0f;
			e->samplesUntilNextSegment = 0x7FFFFFFF;
			e->segmentIsExponential = TSF_FALSE;
			return;
		case TSF_SEGMENT_SUSTAIN:
			e->segment = TSF_SEGMENT_RELEASE;
			e->samplesUntilNextSegment = tsf_voice_envelope_release_samples(e);
			if (e->isAmpEnv)
			{
				// I don't truly understand this; just following what LinuxSampler does.
				float mysterySlope = -9.226f / e->samplesUntilNextSegment;
				e->slope = TSF_EXPF(mysterySlope);
				e->segmentIsExponential = TSF_TRUE;
			}
			else
			{
				e->slope = -e->level / e->samplesUntilNextSegment;
				e->segmentIsExponential = TSF_FALSE;
			}
			return;
		case TSF_SEGMENT_RELEASE:
		default:
			e->segment = TSF_SEGMENT_DONE;
			e->segmentIsExponential = TSF_FALSE;
			e->level = e->slope = 0.0f;
			e->samplesUntilNextSegment = 0x7FFFFFF;
	}
}

static void tsf_voice_envelope_setup(struct tsf_voice_envelope* e, struct tsf_envelope* new_parameters, int midiNoteNumber, short midiVelocity, TSF_BOOL isAmpEnv)
{
	e->parameters = *new_parameters;
	if (e->parameters.keynumToHold)
	{
		e->parameters.hold += e->parameters.keynumToHold * (60.0f - midiNoteNumber);
		e->parameters.hold = (e->parameters.hold < -10000.0f ? 0.0f : tsf_timecents2Secsf(e->parameters.hold));
	}
	if (e->parameters.keynumToDecay)
	{
		e->parameters.decay += e->parameters.keynumToDecay * (60.0f - midiNoteNumber);
		e->parameters.decay = (e->parameters.decay < -10000.0f ? 0.0f : tsf_timecents2Secsf(e->parameters.decay));
	}
	e->midiVelocity = midiVelocity;
	e->isAmpEnv = isAmpEnv;
	tsf_voice_envelope_nextsegment(e, TSF_SEGMENT_NONE);
}

static void tsf_voice_envelope_process(struct tsf_voice_envelope* e, int numSamples)
{
	if (e->slope)
	{
		if (e->segmentIsExponential) e->level *= TSF_POWF(e->slope, (float)numSamples);
		else e->level += (e->slope * numSamples);
	}
	if ((e->samplesUntilNextSegment -= numSamples) <= 0)
		tsf_voice_envelope_nextsegment(e, e->segment);
}

static void tsf_voice_kill(struct tsf_voice* v)
{
	v->playingPreset = -1;
}

static void tsf_voice_end(tsf* f, struct tsf_voice* v)
{
	// if maxVoiceNum is set, assume that voice rendering and note queuing are on separate threads
	// so to minimize the chance that voice rendering would advance the segment at the same time
	// we just do it twice here and hope that it sticks
	int repeats = (f->maxVoiceNum ? 2 : 1);
	while (repeats--)
	{
		tsf_voice_envelope_nextsegment(&v->ampenv, TSF_SEGMENT_SUSTAIN);
		if (v->region->loop_mode == TSF_LOOPMODE_SUSTAIN)
		{
			// Continue playing, but stop looping.
			v->loopEnd = v->loopStart;
		}
	}
}

static void tsf_voice_endquick(tsf* f, struct tsf_voice* v)
{
	// if maxVoiceNum is set, assume that voice rendering and note queuing are on separate threads
	// so to minimize the chance that voice rendering would advance the segment at the same time
	// we just do it twice here and hope that it sticks
	int repeats = (f->maxVoiceNum ? 2 : 1);
	while (repeats--)
	{
		v->ampenv.parameters.release = 0.0f; tsf_voice_envelope_nextsegment(&v->ampenv, TSF_SEGMENT_SUSTAIN);
	}
}

static void tsf_voice_calcpitchratio(struct tsf_voice* v, float pitchShift)
{
	float note = v->playingKey + v->region->transpose + v->region->tune / 100.0f;
	float adjustedPitch = v->region->pitch_keycenter + (note - v->region->pitch_keycenter) * (v->region->pitch_keytrack / 100.0f);
	if (pitchShift) adjustedPitch += pitchShift;
	v->pitchInputTimecents = adjustedPitch * 100.0f;
	v->pitchOutputFactor = v->region->sample_rate / (tsf_timecents2Secsf(v->region->pitch_keycenter * 100.0f) * TSF_SAMPLERATE);
}

static void tsf_voice_render(tsf* f, struct tsf_voice* v, int* outputBuffer)
{
	struct tsf_region* region = v->region;
	short* input = f->fontSamples;
	int* output = outputBuffer;

	// Cache some values, to give them at least some chance of ending up in registers.
	TSF_BOOL isLooping = (v->loopStart < v->loopEnd);
	unsigned int tmpLoopStart = v->loopStart, tmpLoopEnd = v->loopEnd;
	float tmpSampleEndDbl = region->end, tmpLoopEndDbl = tmpLoopEnd + 1.0f;
	float tmpSourceSamplePosition = v->sourceSamplePosition;

	float pitchRatio;
	float noteGain = 0;
	int pitchAccumulator = 0;

	pitchRatio = tsf_timecents2Secsf(v->pitchInputTimecents) * v->pitchOutputFactor;

	noteGain = tsf_decibelsToGain(v->noteGainDB);

	int blockSamples = TSF_RENDER_EFFECTSAMPLEBLOCK;
	int gainMono = noteGain * v->ampenv.level * 256.0f;

	// Update EG.
	tsf_voice_envelope_process(&v->ampenv, blockSamples);

	pitchAccumulator = 0;
	while (blockSamples-- && (tmpSourceSamplePosition + pitchAccumulator * pitchRatio) < tmpSampleEndDbl)
	{
		unsigned int pos = (unsigned int)(tmpSourceSamplePosition + pitchAccumulator * pitchRatio);

		*output++ += ((input[pos] * gainMono) >> 8);

		// Next sample.
		++pitchAccumulator;
		if ((tmpSourceSamplePosition + pitchAccumulator * pitchRatio) >= tmpLoopEndDbl && isLooping) {
			tmpSourceSamplePosition += pitchAccumulator * pitchRatio;
			pitchAccumulator = 0;
			tmpSourceSamplePosition -= (tmpLoopEnd - tmpLoopStart + 1.0f);
		}
	}
	tmpSourceSamplePosition += pitchAccumulator * pitchRatio;

	if (tmpSourceSamplePosition >= tmpSampleEndDbl || v->ampenv.segment == TSF_SEGMENT_DONE)
	{
		tsf_voice_kill(v);
		return;
	}

	v->sourceSamplePosition = tmpSourceSamplePosition;
}

TSFDEF tsf* tsf_load(struct tsf_stream* stream)
{
	tsf* res = TSF_NULL;
	struct tsf_riffchunk chunkHead;
	struct tsf_riffchunk chunkList;
	struct tsf_hydra hydra;
	short* rawBuffer = TSF_NULL;
	tsf_u32 smplCount = 0;

	if (!tsf_riffchunk_read(TSF_NULL, &chunkHead, stream) || !TSF_FourCCEquals(chunkHead.id, "sfbk"))
	{
		//if (e) *e = TSF_INVALID_NOSF2HEADER;
		return res;
	}

	// Read hydra and locate sample data.
	TSF_MEMSET(&hydra, 0, sizeof(hydra));
	while (tsf_riffchunk_read(&chunkHead, &chunkList, stream))
	{
		struct tsf_riffchunk chunk;
		if (TSF_FourCCEquals(chunkList.id, "pdta"))
		{
			while (tsf_riffchunk_read(&chunkList, &chunk, stream))
			{
				#define HandleChunk(chunkName) (TSF_FourCCEquals(chunk.id, #chunkName) && !(chunk.size % chunkName##SizeInFile)) \
					{ \
						int num = chunk.size / chunkName##SizeInFile, i; \
						hydra.chunkName##Num = num; \
						hydra.chunkName##s = (struct tsf_hydra_##chunkName*)TSF_MALLOC(num * sizeof(struct tsf_hydra_##chunkName)); \
						for (i = 0; i < num; ++i) tsf_hydra_read_##chunkName(&hydra.chunkName##s[i], stream); \
					}
				enum
				{
					phdrSizeInFile = 38, pbagSizeInFile =  4, pmodSizeInFile = 10,
					pgenSizeInFile =  4, instSizeInFile = 22, ibagSizeInFile =  4,
					imodSizeInFile = 10, igenSizeInFile =  4, shdrSizeInFile = 46
				};
				if      HandleChunk(phdr) else if HandleChunk(pbag) else if HandleChunk(pmod)
				else if HandleChunk(pgen) else if HandleChunk(inst) else if HandleChunk(ibag)
				else if HandleChunk(imod) else if HandleChunk(igen) else if HandleChunk(shdr)
				else stream->skip(stream->data, chunk.size);
				#undef HandleChunk
			}
		}
		else if (TSF_FourCCEquals(chunkList.id, "sdta"))
		{
			while (tsf_riffchunk_read(&chunkList, &chunk, stream))
			{
				if ((TSF_FourCCEquals(chunk.id, "smpl")
						#ifdef STB_VORBIS_INCLUDE_STB_VORBIS_H
						|| TSF_FourCCEquals(chunk.id, "smpo")
						#endif
					) && !rawBuffer && chunk.size >= sizeof(short))
				{
					if (!tsf_load_samples(&rawBuffer, &smplCount, &chunk, stream)) goto out_of_memory;
				}
				else stream->skip(stream->data, chunk.size);
			}
		}
		else stream->skip(stream->data, chunkList.size);
	}
	if (!hydra.phdrs || !hydra.pbags || !hydra.pmods || !hydra.pgens || !hydra.insts || !hydra.ibags || !hydra.imods || !hydra.igens || !hydra.shdrs)
	{
		//if (e) *e = TSF_INVALID_INCOMPLETE;
	}
	else if (!rawBuffer)
	{
		//if (e) *e = TSF_INVALID_NOSAMPLEDATA;
	}
	else
	{
		res = (tsf*)TSF_MALLOC(sizeof(tsf));
		if (!tsf_load_presets(res, &hydra, smplCount)) goto out_of_memory;
		res->fontSamples = rawBuffer;
		rawBuffer = TSF_NULL; // don't free below
	}
	if (0)
	{
		out_of_memory:
		TSF_FREE(res);
		res = TSF_NULL;
		//if (e) *e = TSF_OUT_OF_MEMORY;
	}
	TSF_FREE(hydra.phdrs); TSF_FREE(hydra.pbags); TSF_FREE(hydra.pmods);
	TSF_FREE(hydra.pgens); TSF_FREE(hydra.insts); TSF_FREE(hydra.ibags);
	TSF_FREE(hydra.imods); TSF_FREE(hydra.igens); TSF_FREE(hydra.shdrs);
	TSF_FREE(rawBuffer);
	return res;
}

TSFDEF tsf* tsf_copy(tsf* f)
{
	tsf* res;
	if (!f) return TSF_NULL;
	if (!f->refCount)
	{
		f->refCount = (int*)TSF_MALLOC(sizeof(int));
		*f->refCount = 1;
	}
	res = (tsf*)TSF_MALLOC(sizeof(tsf));
	TSF_MEMCPY(res, f, sizeof(tsf));
	res->voices = TSF_NULL;
	res->voiceNum = 0;
	res->channels = TSF_NULL;
	(*res->refCount)++;
	return res;
}

TSFDEF void tsf_close(tsf* f)
{
	if (!f) return;
	if (!f->refCount || !--(*f->refCount))
	{
		struct tsf_preset *preset = f->presets, *presetEnd = preset + f->presetNum;
		for (; preset != presetEnd; preset++) TSF_FREE(preset->regions);
		TSF_FREE(f->presets);
		TSF_FREE(f->fontSamples);
		TSF_FREE(f->refCount);
	}
	TSF_FREE(f->channels);
	TSF_FREE(f->voices);
	TSF_FREE(f);
}

TSFDEF void tsf_reset(tsf* f)
{
	struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
	for (; v != vEnd; v++)
		if (v->playingPreset != -1 && (v->ampenv.segment < TSF_SEGMENT_RELEASE || v->ampenv.parameters.release))
			tsf_voice_endquick(f, v);
	if (f->channels) { TSF_FREE(f->channels); f->channels = TSF_NULL; }
}

TSFDEF int tsf_get_presetindex(const tsf* f, int bank, int preset_number)
{
	const struct tsf_preset *presets;
	int i, iMax;
	for (presets = f->presets, i = 0, iMax = f->presetNum; i < iMax; i++)
		if (presets[i].preset == preset_number && presets[i].bank == bank)
			return i;
	return -1;
}

TSFDEF int tsf_get_presetcount(const tsf* f)
{
	return f->presetNum;
}

TSFDEF const char* tsf_get_presetname(const tsf* f, int preset)
{
	return (preset < 0 || preset >= f->presetNum ? TSF_NULL : f->presets[preset].presetName);
}

TSFDEF const char* tsf_bank_get_presetname(const tsf* f, int bank, int preset_number)
{
	return tsf_get_presetname(f, tsf_get_presetindex(f, bank, preset_number));
}

TSFDEF void tsf_set_volume(tsf* f, float global_volume)
{
	f->globalGainDB = (global_volume == 1.0f ? 0 : -tsf_gainToDecibels(1.0f / global_volume));
}

TSFDEF int tsf_set_max_voices(tsf* f, int max_voices)
{
	int i = f->voiceNum;
	int newVoiceNum = (f->voiceNum > max_voices ? f->voiceNum : max_voices);
	f->voices = (struct tsf_voice*)TSF_REALLOC(f->voices, newVoiceNum * sizeof(struct tsf_voice));
	f->voiceNum = f->maxVoiceNum = newVoiceNum;
	for (; i < max_voices; i++)
		f->voices[i].playingPreset = -1;
	return 1;
}

TSFDEF int tsf_note_on(tsf* f, int preset_index, int key, float vel)
{
	short midiVelocity = (short)(vel * 127);
	int voicePlayIndex;
	struct tsf_region *region, *regionEnd;

	if (preset_index < 0 || preset_index >= f->presetNum) return 1;
	if (vel <= 0.0f) { tsf_note_off(f, preset_index, key); return 1; }

	// Play all matching regions.
	voicePlayIndex = f->voicePlayIndex++;
	for (region = f->presets[preset_index].regions, regionEnd = region + f->presets[preset_index].regionNum; region != regionEnd; region++)
	{
		struct tsf_voice *voice, *v, *vEnd; TSF_BOOL doLoop;
		if (key < region->lokey || key > region->hikey || midiVelocity < region->lovel || midiVelocity > region->hivel) continue;

		voice = TSF_NULL, v = f->voices, vEnd = v + f->voiceNum;
		if (region->group)
		{
			for (; v != vEnd; v++)
				if (v->playingPreset == preset_index && v->region->group == region->group) tsf_voice_endquick(f, v);
				else if (v->playingPreset == -1 && !voice) voice = v;
		}
		else for (; v != vEnd; v++) if (v->playingPreset == -1) { voice = v; break; }

		if (!voice)
		{
			if (f->maxVoiceNum)
			{
				// Voices have been pre-allocated and limited to a maximum, try to kill a voice off in its release envelope
				int bestKillReleaseSamplePos = -999999999;
				for (v = f->voices; v != vEnd; v++)
				{
					if (v->ampenv.segment == TSF_SEGMENT_RELEASE)
					{
						// We're looking for the voice furthest into its release
						int releaseSamplesDone = tsf_voice_envelope_release_samples(&v->ampenv) - v->ampenv.samplesUntilNextSegment;
						if (releaseSamplesDone > bestKillReleaseSamplePos)
						{
							bestKillReleaseSamplePos = releaseSamplesDone;
							voice = v;
						}
					}
				}
				if (!voice)
					continue;
				tsf_voice_kill(voice);
			}
			else
			{
				// Allocate more voices so we don't need to kill one off.
				struct tsf_voice* newVoices;
				f->voiceNum += 4;
				f->voices = (struct tsf_voice*)TSF_REALLOC(f->voices, f->voiceNum * sizeof(struct tsf_voice));
				voice = &f->voices[f->voiceNum - 4];
				voice[1].playingPreset = voice[2].playingPreset = voice[3].playingPreset = -1;
			}
		}

		voice->region = region;
		voice->playingPreset = preset_index;
		voice->playingKey = key;
		voice->playIndex = voicePlayIndex;
		voice->noteGainDB = f->globalGainDB - (region->attenuation / 10.0f) - tsf_gainToDecibels(1.0f / vel);

		if (f->channels)
		{
			f->channels->setupVoice(f, voice);
		}
		else
		{
			tsf_voice_calcpitchratio(voice, 0);
			// The SFZ spec is silent about the pan curve, but a 3dB pan law seems common. This sqrt() curve matches what Dimension LE does; Alchemy Free seems closer to sin(adjustedPan * pi/2).
			voice->panFactorLeft  = TSF_SQRTF(0.5f - region->pan);
			voice->panFactorRight = TSF_SQRTF(0.5f + region->pan);
		}

		// Offset/end.
		voice->sourceSamplePosition = region->offset;

		// Loop.
		doLoop = (region->loop_mode != TSF_LOOPMODE_NONE && region->loop_start < region->loop_end);
		voice->loopStart = (doLoop ? region->loop_start : 0);
		voice->loopEnd = (doLoop ? region->loop_end : 0);

		// Setup envelopes.
		tsf_voice_envelope_setup(&voice->ampenv, &region->ampenv, key, midiVelocity, TSF_TRUE);
	}
	return 1;
}

TSFDEF int tsf_bank_note_on(tsf* f, int bank, int preset_number, int key, float vel)
{
	int preset_index = tsf_get_presetindex(f, bank, preset_number);
	if (preset_index == -1) return 0;
	return tsf_note_on(f, preset_index, key, vel);
}

TSFDEF void tsf_note_off(tsf* f, int preset_index, int key)
{
	struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum, *vMatchFirst = TSF_NULL, *vMatchLast = TSF_NULL;
	for (; v != vEnd; v++)
	{
		//Find the first and last entry in the voices list with matching preset, key and look up the smallest play index
		if (v->playingPreset != preset_index || v->playingKey != key || v->ampenv.segment >= TSF_SEGMENT_RELEASE) continue;
		else if (!vMatchFirst || v->playIndex < vMatchFirst->playIndex) vMatchFirst = vMatchLast = v;
		else if (v->playIndex == vMatchFirst->playIndex) vMatchLast = v;
	}
	if (!vMatchFirst) return;
	for (v = vMatchFirst; v <= vMatchLast; v++)
	{
		//Stop all voices with matching preset, key and the smallest play index which was enumerated above
		if (v != vMatchFirst && v != vMatchLast &&
			(v->playIndex != vMatchFirst->playIndex || v->playingPreset != preset_index || v->playingKey != key || v->ampenv.segment >= TSF_SEGMENT_RELEASE)) continue;
		tsf_voice_end(f, v);
	}
}

TSFDEF int tsf_bank_note_off(tsf* f, int bank, int preset_number, int key)
{
	int preset_index = tsf_get_presetindex(f, bank, preset_number);
	if (preset_index == -1) return 0;
	tsf_note_off(f, preset_index, key);
	return 1;
}

TSFDEF void tsf_note_off_all(tsf* f)
{
	struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
	for (; v != vEnd; v++) if (v->playingPreset != -1 && v->ampenv.segment < TSF_SEGMENT_RELEASE)
		tsf_voice_end(f, v);
}

TSFDEF int tsf_active_voice_count(tsf* f)
{
	int count = 0;
	struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
	for (; v != vEnd; v++) if (v->playingPreset != -1) count++;
	return count;
}

TSFDEF void tsf_render_short(tsf* f, short* buffer)
{
	struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
	static int tempbuffer[TSF_RENDER_EFFECTSAMPLEBLOCK];
	TSF_MEMSET(tempbuffer, 0, sizeof(tempbuffer));

	for (; v != vEnd; v++)
		if (v->playingPreset != -1)
			tsf_voice_render(f, v, tempbuffer);

	for (int i = 0; i < TSF_RENDER_EFFECTSAMPLEBLOCK; i++) {
		if (tempbuffer[i] < -32768) {
			*buffer++ = -32768;
		} else if (tempbuffer[i] > 32767) {
			*buffer++ = 32767;
		} else {
			*buffer++ = tempbuffer[i];
		}
	}
}

static void tsf_channel_setup_voice(tsf* f, struct tsf_voice* v)
{
	struct tsf_channel* c = &f->channels->channels[f->channels->activeChannel];
	float newpan = v->region->pan + c->panOffset;
	v->playingChannel = f->channels->activeChannel;
	v->noteGainDB += c->gainDB;
	tsf_voice_calcpitchratio(v, (c->pitchWheel == 8192 ? c->tuning : ((c->pitchWheel / 16383.0f * c->pitchRange * 2.0f) - c->pitchRange + c->tuning)));
	if      (newpan <= -0.5f) { v->panFactorLeft = 1.0f; v->panFactorRight = 0.0f; }
	else if (newpan >=  0.5f) { v->panFactorLeft = 0.0f; v->panFactorRight = 1.0f; }
	else { v->panFactorLeft = TSF_SQRTF(0.5f - newpan); v->panFactorRight = TSF_SQRTF(0.5f + newpan); }
}

static struct tsf_channel* tsf_channel_init(tsf* f, int channel)
{
	int i;
	if (f->channels && channel < f->channels->channelNum) return &f->channels->channels[channel];
	if (!f->channels)
	{
		f->channels = (struct tsf_channels*)TSF_MALLOC(sizeof(struct tsf_channels) + sizeof(struct tsf_channel) * channel);
		f->channels->setupVoice = &tsf_channel_setup_voice;
		f->channels->channelNum = 0;
		f->channels->activeChannel = 0;
	}
	else
	{
		struct tsf_channels *newChannels = (struct tsf_channels*)TSF_REALLOC(f->channels, sizeof(struct tsf_channels) + sizeof(struct tsf_channel) * channel);
		f->channels = newChannels;
	}
	i = f->channels->channelNum;
	f->channels->channelNum = channel + 1;
	for (; i <= channel; i++)
	{
		struct tsf_channel* c = &f->channels->channels[i];
		c->presetIndex = c->bank = 0;
		c->pitchWheel = c->midiPan = 8192;
		c->midiVolume = c->midiExpression = 16383;
		c->midiRPN = 0xFFFF;
		c->midiData = 0;
		c->panOffset = 0.0f;
		c->gainDB = 0.0f;
		c->pitchRange = 2.0f;
		c->tuning = 0.0f;
	}
	return &f->channels->channels[channel];
}

static void tsf_channel_applypitch(tsf* f, int channel, struct tsf_channel* c)
{
	struct tsf_voice *v, *vEnd;
	float pitchShift = (c->pitchWheel == 8192 ? c->tuning : ((c->pitchWheel / 16383.0f * c->pitchRange * 2.0f) - c->pitchRange + c->tuning));
	for (v = f->voices, vEnd = v + f->voiceNum; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel)
			tsf_voice_calcpitchratio(v, pitchShift);
}

TSFDEF int tsf_channel_set_presetindex(tsf* f, int channel, int preset_index)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	c->presetIndex = (unsigned short)preset_index;
	return 1;
}

TSFDEF int tsf_channel_set_presetnumber(tsf* f, int channel, int preset_number, int flag_mididrums)
{
	int preset_index;
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	if (flag_mididrums)
	{
		preset_index = tsf_get_presetindex(f, 128 | (c->bank & 0x7FFF), preset_number);
		if (preset_index == -1) preset_index = tsf_get_presetindex(f, 128, preset_number);
		if (preset_index == -1) preset_index = tsf_get_presetindex(f, 128, 0);
		if (preset_index == -1) preset_index = tsf_get_presetindex(f, (c->bank & 0x7FFF), preset_number);
	}
	else preset_index = tsf_get_presetindex(f, (c->bank & 0x7FFF), preset_number);
	if (preset_index == -1) preset_index = tsf_get_presetindex(f, 0, preset_number);
	if (preset_index != -1)
	{
		c->presetIndex = (unsigned short)preset_index;
		return 1;
	}
	return 0;
}

TSFDEF int tsf_channel_set_bank(tsf* f, int channel, int bank)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	c->bank = (unsigned short)bank;
	return 1;
}

TSFDEF int tsf_channel_set_bank_preset(tsf* f, int channel, int bank, int preset_number)
{
	int preset_index;
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	preset_index = tsf_get_presetindex(f, bank, preset_number);
	if (preset_index == -1) return 0;
	c->presetIndex = (unsigned short)preset_index;
	c->bank = (unsigned short)bank;
	return 1;
}

TSFDEF int tsf_channel_set_pan(tsf* f, int channel, float pan)
{
	struct tsf_voice *v, *vEnd;
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	for (v = f->voices, vEnd = v + f->voiceNum; v != vEnd; v++)
		if (v->playingChannel == channel && v->playingPreset != -1)
		{
			float newpan = v->region->pan + pan - 0.5f;
			if      (newpan <= -0.5f) { v->panFactorLeft = 1.0f; v->panFactorRight = 0.0f; }
			else if (newpan >=  0.5f) { v->panFactorLeft = 0.0f; v->panFactorRight = 1.0f; }
			else { v->panFactorLeft = TSF_SQRTF(0.5f - newpan); v->panFactorRight = TSF_SQRTF(0.5f + newpan); }
		}
	c->panOffset = pan - 0.5f;
	return 1;
}

TSFDEF int tsf_channel_set_volume(tsf* f, int channel, float volume)
{
	float gainDB = tsf_gainToDecibels(volume), gainDBChange;
	struct tsf_voice *v, *vEnd;
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	if (gainDB == c->gainDB) return 1;
	for (v = f->voices, vEnd = v + f->voiceNum, gainDBChange = gainDB - c->gainDB; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel)
			v->noteGainDB += gainDBChange;
	c->gainDB = gainDB;
	return 1;
}

TSFDEF int tsf_channel_set_pitchwheel(tsf* f, int channel, int pitch_wheel)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	if (c->pitchWheel == pitch_wheel) return 1;
	c->pitchWheel = (unsigned short)pitch_wheel;
	tsf_channel_applypitch(f, channel, c);
	return 1;
}

TSFDEF int tsf_channel_set_pitchrange(tsf* f, int channel, float pitch_range)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	if (c->pitchRange == pitch_range) return 1;
	c->pitchRange = pitch_range;
	if (c->pitchWheel != 8192) tsf_channel_applypitch(f, channel, c);
	return 1;
}

TSFDEF int tsf_channel_set_tuning(tsf* f, int channel, float tuning)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	if (c->tuning == tuning) return 1;
	c->tuning = tuning;
	tsf_channel_applypitch(f, channel, c);
	return 1;
}

TSFDEF int tsf_channel_note_on(tsf* f, int channel, int key, float vel)
{
	if (!f->channels || channel >= f->channels->channelNum) return 1;
	f->channels->activeChannel = channel;
	return tsf_note_on(f, f->channels->channels[channel].presetIndex, key, vel);
}

TSFDEF void tsf_channel_note_off(tsf* f, int channel, int key)
{
	struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum, *vMatchFirst = TSF_NULL, *vMatchLast = TSF_NULL;
	for (; v != vEnd; v++)
	{
		//Find the first and last entry in the voices list with matching channel, key and look up the smallest play index
		if (v->playingPreset == -1 || v->playingChannel != channel || v->playingKey != key || v->ampenv.segment >= TSF_SEGMENT_RELEASE) continue;
		else if (!vMatchFirst || v->playIndex < vMatchFirst->playIndex) vMatchFirst = vMatchLast = v;
		else if (v->playIndex == vMatchFirst->playIndex) vMatchLast = v;
	}
	if (!vMatchFirst) return;
	for (v = vMatchFirst; v <= vMatchLast; v++)
	{
		//Stop all voices with matching channel, key and the smallest play index which was enumerated above
		if (v != vMatchFirst && v != vMatchLast &&
			(v->playIndex != vMatchFirst->playIndex || v->playingPreset == -1 || v->playingChannel != channel || v->playingKey != key || v->ampenv.segment >= TSF_SEGMENT_RELEASE)) continue;
		tsf_voice_end(f, v);
	}
}

TSFDEF void tsf_channel_note_off_all(tsf* f, int channel)
{
	struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
	for (; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel && v->ampenv.segment < TSF_SEGMENT_RELEASE)
			tsf_voice_end(f, v);
}

TSFDEF void tsf_channel_sounds_off_all(tsf* f, int channel)
{
	struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
	for (; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel && (v->ampenv.segment < TSF_SEGMENT_RELEASE || v->ampenv.parameters.release))
			tsf_voice_endquick(f, v);
}

TSFDEF int tsf_channel_midi_control(tsf* f, int channel, int controller, int control_value)
{
	struct tsf_channel* c = tsf_channel_init(f, channel);
	if (!c) return 0;
	switch (controller)
	{
		case   7 /*VOLUME_MSB*/      : c->midiVolume     = (unsigned short)((c->midiVolume     & 0x7F  ) | (control_value << 7)); goto TCMC_SET_VOLUME;
		case  39 /*VOLUME_LSB*/      : c->midiVolume     = (unsigned short)((c->midiVolume     & 0x3F80) |  control_value);       goto TCMC_SET_VOLUME;
		case  11 /*EXPRESSION_MSB*/  : c->midiExpression = (unsigned short)((c->midiExpression & 0x7F  ) | (control_value << 7)); goto TCMC_SET_VOLUME;
		case  43 /*EXPRESSION_LSB*/  : c->midiExpression = (unsigned short)((c->midiExpression & 0x3F80) |  control_value);       goto TCMC_SET_VOLUME;
		case  10 /*PAN_MSB*/         : c->midiPan        = (unsigned short)((c->midiPan        & 0x7F  ) | (control_value << 7)); goto TCMC_SET_PAN;
		case  42 /*PAN_LSB*/         : c->midiPan        = (unsigned short)((c->midiPan        & 0x3F80) |  control_value);       goto TCMC_SET_PAN;
		case   6 /*DATA_ENTRY_MSB*/  : c->midiData       = (unsigned short)((c->midiData       & 0x7F)   | (control_value << 7)); goto TCMC_SET_DATA;
		case  38 /*DATA_ENTRY_LSB*/  : c->midiData       = (unsigned short)((c->midiData       & 0x3F80) |  control_value);       goto TCMC_SET_DATA;
		case   0 /*BANK_SELECT_MSB*/ : c->bank = (unsigned short)(0x8000 | control_value); return 1; //bank select MSB alone acts like LSB
		case  32 /*BANK_SELECT_LSB*/ : c->bank = (unsigned short)((c->bank & 0x8000 ? ((c->bank & 0x7F) << 7) : 0) | control_value); return 1;
		case 101 /*RPN_MSB*/         : c->midiRPN = (unsigned short)(((c->midiRPN == 0xFFFF ? 0 : c->midiRPN) & 0x7F  ) | (control_value << 7)); return 1;
		case 100 /*RPN_LSB*/         : c->midiRPN = (unsigned short)(((c->midiRPN == 0xFFFF ? 0 : c->midiRPN) & 0x3F80) |  control_value); return 1;
		case  98 /*NRPN_LSB*/        : c->midiRPN = 0xFFFF; return 1;
		case  99 /*NRPN_MSB*/        : c->midiRPN = 0xFFFF; return 1;
		case 120 /*ALL_SOUND_OFF*/   : tsf_channel_sounds_off_all(f, channel); return 1;
		case 123 /*ALL_NOTES_OFF*/   : tsf_channel_note_off_all(f, channel);   return 1;
		case 121 /*ALL_CTRL_OFF*/    :
			c->midiVolume = c->midiExpression = 16383;
			c->midiPan = 8192;
			c->bank = 0;
			c->midiRPN = 0xFFFF;
			c->midiData = 0;
			tsf_channel_set_volume(f, channel, 1.0f);
			tsf_channel_set_pan(f, channel, 0.5f);
			tsf_channel_set_pitchrange(f, channel, 2.0f);
			tsf_channel_set_tuning(f, channel, 0);
			return 1;
	}
	return 1;
TCMC_SET_VOLUME:
	//Raising to the power of 3 seems to result in a decent sounding volume curve for MIDI
	tsf_channel_set_volume(f, channel, TSF_POWF((c->midiVolume / 16383.0f) * (c->midiExpression / 16383.0f), 3.0f));
	return 1;
TCMC_SET_PAN:
	tsf_channel_set_pan(f, channel, c->midiPan / 16383.0f);
	return 1;
TCMC_SET_DATA:
	if      (c->midiRPN == 0) tsf_channel_set_pitchrange(f, channel, (c->midiData >> 7) + 0.01f * (c->midiData & 0x7F));
	else if (c->midiRPN == 1) tsf_channel_set_tuning(f, channel, (int)c->tuning + ((float)c->midiData - 8192.0f) / 8192.0f); //fine tune
	else if (c->midiRPN == 2 && controller == 6) tsf_channel_set_tuning(f, channel, ((float)control_value - 64.0f) + (c->tuning - (int)c->tuning)); //coarse tune
	return 1;
}

TSFDEF int tsf_channel_get_preset_index(tsf* f, int channel)
{
	return (f->channels && channel < f->channels->channelNum ? f->channels->channels[channel].presetIndex : 0);
}

TSFDEF int tsf_channel_get_preset_bank(tsf* f, int channel)
{
	return (f->channels && channel < f->channels->channelNum ? (f->channels->channels[channel].bank & 0x7FFF) : 0);
}

TSFDEF int tsf_channel_get_preset_number(tsf* f, int channel)
{
	return (f->channels && channel < f->channels->channelNum ? f->presets[f->channels->channels[channel].presetIndex].preset : 0);
}

TSFDEF float tsf_channel_get_pan(tsf* f, int channel)
{
	return (f->channels && channel < f->channels->channelNum ? f->channels->channels[channel].panOffset - 0.5f : 0.5f);
}

TSFDEF float tsf_channel_get_volume(tsf* f, int channel)
{
	return (f->channels && channel < f->channels->channelNum ? tsf_decibelsToGain(f->channels->channels[channel].gainDB) : 1.0f);
}

TSFDEF int tsf_channel_get_pitchwheel(tsf* f, int channel)
{
	return (f->channels && channel < f->channels->channelNum ? f->channels->channels[channel].pitchWheel : 8192);
}

TSFDEF float tsf_channel_get_pitchrange(tsf* f, int channel)
{
	return (f->channels && channel < f->channels->channelNum ? f->channels->channels[channel].pitchRange : 2.0f);
}

TSFDEF float tsf_channel_get_tuning(tsf* f, int channel)
{
	return (f->channels && channel < f->channels->channelNum ? f->channels->channels[channel].tuning : 0.0f);
}

#ifdef __cplusplus
}
#endif

#endif //TSF_IMPLEMENTATION
