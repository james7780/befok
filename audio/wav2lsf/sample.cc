/*********************************
 * common sample helper routines *
 *********************************/

/*
	42BS      42Bastian Schick (elw5basc@gp.fht-esslingen.de)

	history :
	YY/MM/DD
	98/01/06-     42BS    created
*/

#include "g.h"
#include "sample.h"

void Histogram(tCommonHead &smp, ushort histo[], uchar &min, uchar &max)
{
	uchar _min = 255;
	uchar _max = 0;
	uchar c;
	uchar * data = smp.data;
	long len = smp.length;

	//bzero(histo,256*sizeof(ushort));
	memset(histo, 0, 256 * sizeof(ushort));

	while (len--)
		{
		++histo[c = *data++];
		if (c > _max) _max = c;
		if (c < _min) _min = c;
		}
	min = _min;
	max = _max;
}

void ChangeVolume(tCommonHead &smp, ushort volumeScale)
{
	uchar *data = smp.data;
	long len = smp.length;

	while (len--)
		{
		ushort val = (*data) * volumeScale / 100;
		*data++ = val < 256 ? val : 255;
		}
}

void Change2Signed(tCommonHead &smp)
{
	uchar * data = smp.data;
	long len = smp.length;

	while (len--)
		*data++ ^= 0x80;
}

long SamplePacker(tCommonHead &sample, uchar **data)
{
	// JH - Made local const
	const char power[] = {-128, -64, -32, -16, -8, -4, -2, -1, 0, 1, 2, 4, 8, 16, 32, 64};
	const short times[] = {128, 64, 32, 16, 8, 4, 2, 1, 1, 1, 2, 4, 8, 16, 32, 64};

	char last;
	short delta;
	uchar nibble1, nibble2;
	uchar *ptr;
	uchar *unpacked = sample.data;
	long len = ((sample.length + 1) & 0xffffffe) / 2;

	if ((*data = ptr = (uchar *)malloc(len)) == 0)
		return (-1);

	uchar nibble_encode[256 * 2];
	uchar *pnibble = nibble_encode;
	for (short i = 0; i < 16; ++i)
		for (short j = times[i]; j; --j)
			*pnibble++ = i;

	//bzero(pnibble,128);
	memset(pnibble, 0, 128);

	if (sample.stereo)
		{
		// not yet
		}
	else
		{
		last = 0;
		long len2 = len;
		while (len2--)
			{
			delta = (short)*(char *)unpacked++ - (short)last;
			nibble1 = nibble_encode[delta + 255];
			last += power[nibble1];

			delta = (short)*(char *)unpacked++ - (short)last;
			nibble2 = nibble_encode[delta + 255];
			last += power[nibble2];

			*ptr++ = (nibble1 << 4) | nibble2;
			}
		}
	return len;
}
/****************************************/

void ShowHistogram(tCommonHead &sample)
{
	ushort histo[256];
	ushort *phisto = histo;
	uchar min, max;

	Histogram(sample, histo, min, max);

	printf("\n Min : %3d  Max : %3d \n", min, max);
	for (short i = 16; i; --i)
		{
		for (short j = 16; j; --j)
			{
			printf("%4d ", *phisto++);
			}
		//    printf("\n");
		}
}
