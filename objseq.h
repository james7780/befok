// object sequencing information
// format: time (16 bits), object (8 bits)
// time = frames / 10 (?)
#define SROCK	1
#define BROCK	2
#define SHOLE	3
#define BHOLE	4
#define SAUCER	5
#define UFO		6
#define BOMBER	7
// + more here

unsigned char objseq1[300] = {
	  5,	0,	SROCK,
	 20,	0,	SROCK,
	 30,	0,	SROCK,
	 100,	0,	BROCK,
	 110,	0,	BROCK,
	 200,	0,	SHOLE,
	 220,	0,	BHOLE,
	 255, 255, 	0
};
	
