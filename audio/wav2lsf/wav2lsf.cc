/************************
 * WAV to LNX converter *
 * with sample.packer   *
 ************************/

// Updated JH 2019-03-24

#define VER "1.1"

#include "g.h"
#include "sample.h"

struct SWavSmpHead
{
	long Magic;       // 'fmt '
	long chunklen;    // without chunk-header : here 16
	short weisnich;
	short stereo;     // 1 = Mono 2 = Stereo
	ulong playfrq;
	ulong smplfrq;
	short weisnich2;
	short bps;        // 8 or 16
};

struct SLnxSmpHead
{
	uchar stereo;     // != 0 stereo
	uchar length[2];  // NOTE : big endian
	uchar reload;     // Timer reload : 1MHz/sample_frq - 1
	uchar unpacked;   // != 1 unpacked data
};

ushort NoWave(uchar *in, long len);
long SaveLsf(char *name, tCommonHead &sample, bool packed);

/* some helpers */
void error(char * s, char *s1);
ulong ReverseEndian(ulong d);

//
// do the nasty little/big-endian thing
// (Is there any >16bit CPU besides x86-based that use LE ??)
//
#if BYTE_ORDER == LITTLE_ENDIAN
#define LE2BE(a)   ReverseEndian(a)
#define BE2LE(a)   (a)
#else
#define LE2BE(a)   (a)
#define BE2LE(a)   ReverseEndian(a)
#endif

/* loadsave.cc */
//long LoadFile(char *fn,uchar ** data);
//long SaveFile(char *fn,uchar * data,ulong len,uchar *head,ulong headlen);

// IMplemented JH 2019-02-28
unsigned long LoadFile(const char *filename, uchar **data)
{
	unsigned long len;

	//if ((f = fopen(fn, O_RDONLY | O_BINARY)) >= 0)
	// JH 2018-05-11 - Modified to avoid crash in Visual Studio
	FILE *f = fopen(filename, "rb");
	if (f)
		{
		fseek(f, 0L, SEEK_END);
		len = ftell(f);
		fseek(f, 0L, SEEK_SET);
		if ((*data = (uchar *)malloc(len)) == NULL)
			return 0;
#ifdef _DEBUG
		printf("filesize: %lu\n", len);
#endif
		len = fread(*data, 1, len, f);
#ifdef _DEBUG
		//printf("sizeof(int): %u\n", sizeof(int));
		printf("bytes read: %lu\n", len);
#endif
		fclose(f);
		//if (verbose) printf("Read: %s \n", filename);

		return (len);
		}
	else
		return 0;
}

// IMplemented JH 2019-02-28
void SaveFile(char *filename, uchar *data, ulong len, uchar *head, ulong headlen)
{
	FILE *f = fopen(filename, "wb");
	if (f)
		{
		fwrite(head, 1, headlen, f);
		fwrite(data, 1, len, f);
		}
	fclose(f);
}

/// Implemented JH 2019-03-24
/// Saves the output LSF to a .h or .c file (for streamlined workflow)
void SaveCFile(char *filename, uchar *data, ulong len, uchar *head, ulong headlen)
{
	FILE *f = fopen(filename, "w");
	if (f)
		{
		// Write C style data declaration
		// Find filename from path, to use as C data reference name
		char s[1000];
		int lastSlashPos = strlen(filename);
		while(lastSlashPos >= 0)
			{
			if (filename[lastSlashPos] == '/' || filename[lastSlashPos] == '\\')
				break;
			lastSlashPos--;
			}
		strcpy(s, filename + lastSlashPos + 1);
		// Remove extension from name
		char *ext = strchr(s, '.');
		if (ext > 0)
			*ext = 0; 
		fprintf(f, "unsigned char %s[%d] = {\n", s, headlen + len);
		// Write header bytes
		fprintf(f, "\t");
		for (ulong i = 0; i < headlen; i++)
			fprintf(f, "0x%02X, ", head[i]);
		// Write data bytes
		for (ulong i = 0; i < len; i++)
			{
			if (0 == i % 8)
				fprintf(f, "\n\t");
			if (i == len - 1)
				fprintf(f, "0x%02X", data[i]);
			else
				fprintf(f, "0x%02X, ", data[i]);

			}
		fprintf(f, "\n};\n");
		}
	fclose(f);
}


ushort NoWave(uchar *in, long len)
{

	if (LE2BE(*(long *)in) != 'RIFF')
		return 1;

	if (BE2LE(*(long *)(in + 4)) + 8 != len)
		return 1;


	if (LE2BE(*(long *)(in + 8)) != 'WAVE')
		return 1;

	return 0;
}


/// Convert WAV to internal sample format
void WaveToCommon(uchar *in, long len, tCommonHead &sample)
{
	uchar * help = in + 12; // pointer behind 'WAVE' (or "RIFF")

	while (LE2BE(*(long *)help) != 'data')
		help += 8 + BE2LE(*(long *)(help + 4));

	long sample_len = BE2LE(*(long *)(help + 4));
	uchar *sample_data = help + 8;

	help = in + 12;

	while (LE2BE(*(long *)help) != 'fmt ')
		help += 8 + BE2LE(*(long *)(help + 4));

	struct SWavSmpHead * wavhead = (struct SWavSmpHead *)help;

	sample.length = sample_len;					// Length of sample data (whether 8 or 16 bit)
	sample.frq = BE2LE(wavhead->smplfrq);		// Number of bytes played back per second
	sample.data = sample_data;
	sample.stereo = (wavhead->stereo == 0x0200) || (wavhead->stereo == 2);
	sample.bits = wavhead->bps;					// JH 2019-03-24
}

/// Save internal sample to LSF binary or C-file (ASCII) format
long SaveLsf(char *filename, tCommonHead &sample, bool packed, bool write_c_file)
{
	struct SLnxSmpHead head;
	uchar * d;
	long l;

// JH - This seems to be for 8-bit WAVs?
	Change2Signed(sample);

	head.stereo = (uchar)sample.stereo;
	head.reload = (uchar)(1e6 / sample.frq) - 1;

	if (packed)
		{
		if ((l = SamplePacker(sample, &d)) < 0)
			return -1;

		head.length[0] = (l >> 8) & 0xff;
		head.length[1] = (l & 0xff);
		head.unpacked = 0;
		SaveFile(filename, d, l, (uchar *)&head, sizeof(struct SLnxSmpHead));
		}
	else
		{
		// Handle 16-bit WAV samples
//#define WAV16
#ifdef WAV16
		sample.length /= 2;
		head.length[0] = (sample.length >> 8) & 0xff;
		head.length[1] = (sample.length & 0xff);
#else
		head.length[0] = (sample.length>>8)& 0xff;
		head.length[1] = (sample.length & 0xff);
#endif
		head.unpacked = 1;

		// Handle 16-bit WAV samples
#ifdef WAV16
		uchar *data8 = (uchar *)malloc(sample.length);
		for (int i = 0; i < sample.length; i++)
			//data8[i] = ((short *)sample.data)[i] / 256;
			data8[i] = sample.data[i*2];		// TODO - Fix!

		if (write_c_file)
			SaveCFile(filename, data8, sample.length, (uchar *)&head, sizeof(struct SLnxSmpHead));
		else
			SaveFile(filename, data8, sample.length, (uchar *)&head, sizeof(struct SLnxSmpHead));
		free(data8);
#else
		if (write_c_file)
			SaveCFile(filename, sample.data, sample.length,(uchar *)&head,sizeof(struct SLnxSmpHead));
		else
			SaveFile(filename, sample.data, sample.length,(uchar *)&head,sizeof(struct SLnxSmpHead));
#endif
		}
}


void main(int argc, char **argv)
{
	uchar *in, *out;
	long in_len;
	char *in_name = 0;
	char *out_name = 0;
	bool write_c_file = false;
	bool packed = false;
	bool showHistogram = false;
	short volumeScale = 0;
	int offset = 0;

	if (1 == argc)
		error("Usage : wav2lsf [-c] [-h] [-v percentage] -o out in" \
				"\n-c              Ouput as C file" \
				"\n-h              Show histogram of sample" \
				"\n-v percentage   Adjust output volume" \
				"\n-o output_file  Specify the output file name" \
				, "");

	--argc; ++argv;

	while (argc)
		{
		if (0 == strcmp(*argv, "-c"))
			{
			--argc; ++argv;
			write_c_file = true;
			continue;
			}
		if (0 == strcmp(*argv, "-p"))
			{
			--argc; ++argv;
			packed = true;
			continue;
			}
		if (0 == strcmp(*argv, "-h"))
			{
			--argc; ++argv;
			showHistogram = true;
			continue;
			}
		if (0 == strcmp(*argv, "-off"))
			{
			--argc; ++argv;
			if (argc)
				{
				offset = atoi(*argv);
				--argc; ++argv;
				}
			else
				error("Error: Offset value missing !", "");
			continue;
			}

		if (0 == strcmp(*argv, "-o"))
			{
			--argc; ++argv;
			if (argc)
				{
				out_name = *argv;
				--argc; ++argv;
				continue;
				}
			error("Error: Missing output file after -o !\nAborting ...", "");
			}
		if (0 == strcmp(*argv, "-v"))
			{
			--argc; ++argv;
			if (argc)
				{
				volumeScale = atoi(*argv);
				--argc; ++argv;
				continue;
				}
			else
				error("Error: Missing ratio for -v !\n", "");
			}

		if (**argv == '-')
			error("Error: Unknown option.\nAborting ...", "");

		if (in_name)
			error("Error: Multiple input-files.\nAborting ...", "");

		in_name = *argv;
		--argc; ++argv;
		}

	if ((in_len = LoadFile(in_name, &in)) <= 0)
		error("Error: Couldn't load %s !", in_name);

	if (NoWave(in + offset, in_len - offset))
		{
		free(in);
		error("Error: This is no WAV-file !", "");
		}

	tCommonHead sample;		// JH 2019-3-24 - Made "workHeader" a local variable (renamed to "sample" to avoid confusion)
	WaveToCommon(in + offset, in_len - offset, sample);
	printf("WAV length:%d\n", sample.length);
	printf("WAV freq:%d\n", sample.frq);
	printf("WAV bits:%d\n", sample.bits);

	// JH 2019-03-24 - 16-bit WVA files not handled (yet)
	if (sample.bits != 8)
		{
		free(in);
		error("Error: Only 8-bit WAV files can be converted !", "");
		}

	if (showHistogram)
		ShowHistogram(sample);

	if (volumeScale)
		ChangeVolume(sample, volumeScale);

	if (showHistogram)
		ShowHistogram(sample);

	if (out_name)
		SaveLsf(out_name, sample, packed, write_c_file);

	free(in);
}

ulong ReverseEndian(register ulong d)
{
	return  ((d << 24) |
		((d & 0x0000ff00) << 8) |
		((d & 0x00ff0000) >> 8) |
		(d >> 24));
}

void error(char *s, char *s1)
{
	printf("wav2lsf "VER" %s\n", __DATE__);
	printf(s, s1);
	printf("\n");
	exit(-1);
}
