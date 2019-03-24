// audio.h - Defines for the audio part of befok
// JH 2018
#define NULL 0
typedef unsigned char uchar;
typedef unsigned int uint;

#define NUM_CHANNELS	4

//Instrument = struct with reg settings, and vol/pitch envelope:
//	+ Settings (feedback, shifter, counter, clock)
//	+ Envelope length
//	+ Loop flag
//	+ Vol envelope data
//	+ Pitch envelope data
#define ENVELOPE_SIZE	32
typedef struct INSTRUMENT {
	uint shifter;					// shift bits
	uint feedback;					// feedback bits
	uchar integrate;				// 0 or 1
	uchar octave;					// clock select 6 = lowest octave, 0 = highest
	uchar loopLength;				// Envelope loop length (0 for no loop)
	uchar envPos;					// Current position in envelope
	uchar volData[ENVELOPE_SIZE];	// Volume envelope
	uchar pitchData[ENVELOPE_SIZE];	// Pitch envelope
};

// Sound system API (functions)
extern void InitSound(void);
extern void StartSound(uchar channel, struct INSTRUMENT *instrument, char vol, uchar freq);
extern void EndSound(uchar channel);
extern void EndAllSound(void);
extern void UpdateSound(void);
extern void PlaySample(uchar *sampleData);
