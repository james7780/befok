// Befok game sound control/system
// JH 2018
#include "audio.h"
#include <lynx.h>

#define ENABLE_AUDIO

struct INSTRUMENT *channelInstrument[NUM_CHANNELS];

/// Initialise sound output
void InitSound(void)
{
#ifdef ENABLE_AUDIO
	// Channel allocation

	//Procedure for setting up a channel to play a tone Starting from a reset:
	//Wait 100ms (for hardware to initialise)
	//Disable channel
	//Stop counter for channel
	//Write backup (reload) value to channel
	//Write counter value to channel
	//Write shift register bit pattern to channel (coutner must be disabled)
	//Write feedback register bit pattern to channel (must be >0 to get sound!)
	//Enable counter and set timer pre-selector
	//Enable channel

	unsigned int feedback = 0x0019;			// JH - actually feedback bits
	unsigned int shift = 0x00B1;			// JH - actually shift bits
	unsigned int octave = 5;
	unsigned int integrate = 1;
	unsigned char pitch = 4;

	// stop counter for channel
	MIKEY.channel_a.control = 0x10;

	// Write feedback for channel
	MIKEY.channel_a.feedback = (feedback & 0x003f) + ((feedback >> 4) & 0xc0);

	// write backup (reload) value to channel
	MIKEY.channel_a.reload = pitch;
	MIKEY.channel_a.count = pitch;

	// Set up shift reg
	MIKEY.channel_a.shiftlo = shift & 0xFF;
	MIKEY.channel_a.other = (shift >> 4) & 0xf0;

	// Set feedback and enable counter
	//MIKEY.channel_a.control = 0x19; //(feedback & 0x0080) + 0x18 + octave + (integrate << 5);
	MIKEY.channel_a.control = (feedback & 0x0080) + 0x18 + octave + (integrate << 5);

// test
	MIKEY.channel_a.volume = 63;
	MIKEY.mstereo = 0x00;
	//MIKEY.attena = 0x80;

	//for (pitch = 2; pitch < 80; pitch++)
	//	{
	//	MIKEY.channel_a.reload = pitch;
	//	VSYNC(); VSYNC(); VSYNC();
	//	}

	// stop counter for channel
	MIKEY.channel_a.control = 0x10;

	// Reset channel insturment assignments
	channelInstrument[0] = NULL;
	channelInstrument[1] = NULL;
	channelInstrument[2] = NULL;
	channelInstrument[3] = NULL;
#endif
}

/// Play sound using instr n on channel n
void StartSound(uchar channel, struct INSTRUMENT *instrument, char vol, uchar freq)
{
#ifdef ENABLE_AUDIO
	struct _mikey_audio *channelRegs;
	uint feedback;
	//uchar integrate = 0;

	if (channel < NUM_CHANNELS)
		{
		channelInstrument[channel] = instrument;
		channelRegs = &MIKEY.channel_a + channel;

		if (instrument)
			{
			instrument->envPos = 0;
			// Set up shift reg
			channelRegs->shiftlo = instrument->shifter & 0xFF;
			channelRegs->other = (instrument->shifter >> 4) & 0xf0;

			// Write feedback for channel
			feedback = instrument->feedback;
			channelRegs->feedback = (feedback & 0x003f) + ((feedback >> 4) & 0xc0);

			channelRegs->volume = instrument->volData[0];
			channelRegs->reload = instrument->pitchData[0];
			channelRegs->count = instrument->pitchData[0];

			//channelRegs->control = 0x1E;		// 64us (low octave)
			channelRegs->control = (feedback & 0x0080) + 0x18 + instrument->octave + (instrument->integrate << 5);
			}
		else
			{
			channelRegs->control = 0x10;		// stop counter for this channel
			}
		}

/* WAS:
	MIKEY.channel_a.control = 0x1E;		// 64us (low octave)
	//MIKEY.channel_a.reload = 2;
	instrZapper.envPos = 0;
	MIKEY.channel_a.volume = instrZapper.volData[0];
	MIKEY.channel_a.reload = instrZapper.pitchData[0];
*/
#endif
}

/// Stop sound on channel n
void EndSound(uchar channel)
{
#ifdef ENABLE_AUDIO
	struct _mikey_audio *channelRegs;
	struct INSTRUMENT *instrument;

	if (channel < NUM_CHANNELS)
		{
		channelRegs = &MIKEY.channel_a + channel;
		// TODO! - Check pointer arithmetic!!!

		// stop counter for channel
		channelRegs->control = 0x10;

		// Stop instrument (so UpdateSound does not process it)
		instrument = channelInstrument[channel];
		if (instrument)
			instrument->envPos = 0xFF;
		}
#endif
}

/// Stop sound on all channels
void EndAllSound(void)
{
	EndSound(0);
	EndSound(1);
	EndSound(2);
	EndSound(3);
}


/// Update sound output from envelopes
/// Call once a frame
void UpdateSound(void)
{
#ifdef ENABLE_AUDIO
	uchar channel;
	uchar envPos;
	struct _mikey_audio *channelRegs;
	struct INSTRUMENT *instrument;

	for (channel = 0; channel < NUM_CHANNELS; channel++)
		{
		channelRegs = &MIKEY.channel_a + channel;
		// TODO! - Check pointer arithmetic!!!

		instrument = channelInstrument[channel];
		if (NULL == instrument)
			continue;

		envPos = instrument->envPos + 1;
		if (0 == envPos)	// Instrument envPos = 0xFF, meaning not active
			continue;

		if (envPos < ENVELOPE_SIZE)
			{
			// Update sound hardware from the envelope data
			channelRegs->volume = instrument->volData[envPos];
			channelRegs->reload = instrument->pitchData[envPos];
			// increment envelope position
			instrument->envPos = envPos;
			}
		else
			{
			// We have reached the end of the envelope
			instrument->envPos = 0xFF;		// deactivate this instrument
			EndSound(channel);					// switch off sound on this channel
			}
		}
#endif
}

/// Play a sample on channel 3  (***BLOCKING***)
void PlaySample(uchar *sampleData)
{
#ifdef ENABLE_AUDIO
	uchar *dataPos;
	unsigned int count;
	uchar j;
	uchar divider;
	dataPos = sampleData;
	dataPos++;
	count = *dataPos << 8;
	dataPos++;
	count |= *dataPos;
	dataPos++;
	divider = *dataPos;
	dataPos = sampleData + 5;

	MIKEY.channel_d.reload = 0;
	MIKEY.channel_d.control = 0x10;
	MIKEY.channel_d.volume = 127;
	while (--count)
		{
		MIKEY.channel_d.dac = *dataPos;
		dataPos++;
		// wait 200 cycles (for 5000 Hz)
		//j = 12;
		j = divider >> 4;			// tune for pitch
		while (j--)
			{
			asm("nop");			// yeah yeah i know this will be much more than 200 cycles!
			}
		}
	MIKEY.channel_d.volume = 0;
#endif
}

