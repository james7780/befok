// Berzerk for Lynx
// Jum Hig 2001 - 2018
// TODO - Remove tgi_ stuff


#include <stdlib.h>
#include <string.h>
#include <lynx.h>
#include <tgi.h>
#include <6502.h> 

//#define NULL 0
//typedef unsigned char uchar;
//typedef unsigned int uint;
#include "audio.h"

#include "actors.h"

/* __ALWAYS__ use both defines but using is recommended for shorter code */
#define FIXARGC         /* be sure you don't use <8 arguments ! */
#define NOARGC

//#define COLLBUF     0x9C60
//#define BUFFER1     0xBC40
//#define BUFFER2     0xDC20
// tgi locations
#define COLLBUF     0xA058
#define BUFFER1     0xC038
#define BUFFER2     0xE018

#define PEEK(a) *(uchar *)(a)
#define POKE(a, b) *(uchar *)(a) = (b)

uchar *RenderBuffer = (uchar *)(BUFFER1);

// DEBUG
uchar bgtestcol;

// Zapper gun instrument/sfx
struct INSTRUMENT instrZapper = {
	0x00B1,
	0x0010,
	0,			// no integration
	5,			// octave 5 (6 is lowest octave, 0 is highest)
	0,			// no looping
	0,
	{ 64, 60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16, 12, 8, 4, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

//Explosion:
//	+ Shift = 0x4B      (Glenn bit, borrow in, borrow out)
//	+ Low shift = 0x00
//	+ Backup (reload) = 0x00
//	+ Flags = 0x1E      (enable reload, enable count, clock = 64us)

// PLayer hit instrument/sfx
struct INSTRUMENT instrHit = {
	0x004B,
	0x0019,
	0,			// no integration
	6,			// octave 5 (6 is lowest octave, 0 is highest)
	0,			// no looping
	0,
	{ 63, 62, 1, 1, 55, 53, 1, 1, 47, 45, 1, 1, 39, 37, 1, 1, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 20, 20, 20, 20, 30, 30, 30, 30, 45, 45, 45, 45, 60, 60, 60, 60,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

// Otto bounce instrument/sfx
struct INSTRUMENT instrBounce = {
	0x00B1,
	0x0010,
	0,			// no integration
	5,			// octave 5 (6 is lowest octave, 0 is highest)
	0,			// no looping
	0,
	{ 64, 56, 48, 40, 32, 24, 16, 8, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 8, 16, 24, 32, 32, 24, 16, 8, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

// Robot explode instrument/sfx
struct INSTRUMENT instrExplode = {
	0x09A6,
	0x0777,
	0,			// no integration
	6,			// octave 5 (6 is lowest octave, 0 is highest)
	0,			// no looping
	0,
	{ 64, 60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16, 12, 8, 4,  
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100, 100, 100, 100, 100,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

//// Functions from the old Lynx lib that need to be re-implemented
//void SmpInit(uchar, uchar)		// 3, 7);		// init sample player to use channel 3, timer 7
//{
//}
//
//void EnableIRQ(uchar)			//7);
//{
//}
//
//void SmpStart(uchar *, uchar)			//smp_intruder, 0);
//{
//}

uchar random()
{
	return (uchar)rand();
}

/*************** for OBJECTS *************************/
#define MAX_ROBOTS		10				// 8
#define MAX_BULLETS		7				// 2 player + 5 enemy
#define NUM_OBJS		18				// otto + 10 robots + 7 bullets
struct ACTOR_TYPE obj[NUM_OBJS];

// Level data struct
typedef struct LEVELDATA {
	int startScore;			// Score threshold for this level
	uchar numRobots;		// Number of robots on this level
	uchar numBullets;		// Number of bullets fired by robots
	uchar bulletSpeed;		// Speed of bullets (between 4 and 12)
	uchar robotR;			// Colour of robots
	uchar robotG;
	uchar robotB;
};

	// From wiki: the sequence goes as follows:
	// Level	Points		Behaviour
	// 0		0-500		Dark yellow robots that do not fire (orangy-yellow)
	// 1		500-1500	Red robots that can fire 1 bullet (500 points)
	// 2		1500-3000	Dark cyan robots that can fire 2 bullets (1,500 points)
	// 3		3000-4500	Green robots that fire 3 bullets (3k)
	// 4		4500-6000	Dark purple robots that fire 4 bullets (4.5k)
	// 5		6000-7500	Light yellow robots that fire 5 bullets (6k)
	// 6		7500-10k	White robots that fire 1 fast bullet (7.5k)
	// 7		10k-11k		Dark cyan robots that fire 2 fast bullets (10k)
	// 8		11k-13k		Light purple robots that fire 3 fast bullets (11k)
	// 9		13k-15k		Gray robots that fire 4 fast bullets (13k)
	// 10		15k-17k		Dark yellow robots that fire 5 fast bullets (15k)
	// 11		17k-19k		Red robots that fire 5 fast bullets (17k)
	// 12		19k-???		Light cyan robots that fire 5 fast bullets (19k)
#define NUM_LEVELS	13
struct LEVELDATA levelData[NUM_LEVELS]	=	{
	{ 0,     6,	0,  4, 0xE0, 0xD0, 0x20 },				// Dark yellow robots, no bullets
	{ 500,   7, 1,  6, 0xD0, 0x20, 0x00 },				// Red robots, 1 bullet
	{ 1500,  8, 2,  6, 0x20, 0x20, 0xA0 },				// Dark cyan robots, 2 bullets
	{ 3000,  9, 3,  6, 0x00, 0xF0, 0x00 },				// Green robots, 3 bullets
	{ 4500,  MAX_ROBOTS, 4,  6, 0xA0, 0x20, 0xA0 },		// Dark purple 
	{ 6000,  MAX_ROBOTS, 5,  6, 0xF0, 0xF0, 0x00 },		// Light yellow
	{ 7500,  MAX_ROBOTS, 1, 10, 0xF0, 0xF0, 0xF0 },		// White
	{ 10000, MAX_ROBOTS, 2, 10, 0x20, 0x20, 0xA0 },		// Dark cyan
	{ 11000, MAX_ROBOTS, 3, 10, 0xF0, 0x40, 0xF0 },		// Light purple
	{ 13000, MAX_ROBOTS, 4, 10, 0x90, 0x90, 0x90 },		// Grey
	{ 15000, MAX_ROBOTS, 5, 10, 0xE0, 0xD0, 0x20 },		// Dark yellow
	{ 17000, MAX_ROBOTS, 5, 10, 0xF0, 0x00, 0x00 },		// Red
	{ 19000, MAX_ROBOTS, 5, 10, 0x80, 0x80, 0xF0 }		// Light cyan
};



/*************** for VSYNC handling*******************/
uchar frames, lines;

/*************** for MUSIC sequencing ****************/
//uchar *track[4];			// track data pointer
//uchar *vestart[4];			// volume envelope start pointers
//uchar *ve[4];				// volume envelope pointers
//uchar note_vol[4];			// note volumes per track
//uchar trk_on[4];			// track active
//uint seqtime;				// global sequence counter
//uint seqlength;				// sequence length


// Exit numbers
enum { EXIT_LEFT, EXIT_TOP, EXIT_RIGHT, EXIT_BOTTOM };

uint pl_x, pl_y;
uchar last_exit = EXIT_RIGHT;		// for blocking extrance (0,1,2,3 = exited left, top, right, bottom)
uchar exited;

/*************** for OBJECT sequencing ***************/
//uint objseqtick;			// global object sequence counter
//uchar *objseqdata;			// pointer to object sequence data

//char score[5] = { '0', '0', '0', '0', 0 };

// #asm
// _VBLflag = $a0
// #endasm
uchar VBLflag = 0xa0;

//#define VSYNC {++VBLflag;while( VBLflag );}
void VSYNC()
{
	// wait for Mikey timer 2 (VCount)
	//while (MIKEY.timer2.count != 0) ;
	while (_VBL_TIMER.count != 0) ;

	++VBLflag;
}

// Wait for a certain number of frames
void WaitFrames(int frames)
{
	int i;
	for (i = 0; i < frames; i++)
		{
		while (_VBL_TIMER.count != 0)
			{
			asm("nop");
			asm("nop");
			asm("nop");
			asm("nop");
			asm("nop");
			}
		while (_VBL_TIMER.count == 0)
			{
			asm("nop");
			asm("nop");
			asm("nop");
			asm("nop");
			asm("nop");
			}
		}
}

extern uchar SCBsc1[];

/*************** audio samples **************************************/
//extern uchar starkick[];
//extern char robot[];

/*************** sprites ********************************************/
extern char playfield[];			// background sprite data
extern char laser_up[];
extern char laser_lr[];
extern char laser_diag1[];
extern char laser_diag2[];

//#include "images/berzerk_title.h"	// Title sprite data
#include "sprites_new.h"			// sprite data for robots, play, otto
#include "sprites.c"                // sprite data for digits
#include "explosions.c"				// sprite data for explosions

#include "audio/smp_intruder.c"
#include "audio/smp_alert.c"
#include "audio/smp_chicken.c"
//#include "audio/smp_fight.c"
//#include "audio/smp_like.c"
//#include "audio/smp_aye.c"
//#include "audio/smp_robot.c"
#include "audio/smp_destroy.c"
//#include "audio/smp_humanoid.c"

unsigned char *animframe_player_left[6] = { player_left1, player_left2, player_left3, player_left4, player_left5, player_left6 };
unsigned char *animframe_player_up[6] = { player_up1, player_up2, player_up3, player_up4, player_up5, player_up6 };
unsigned char *animframe_player_down[6] = { player_down1, player_down2, player_down3, player_down4, player_down5, player_down6 };
unsigned char *animframe_robot[4] = { robot1, robot2, robot3a, robot4 };
unsigned char *animframe_robot_up[4] = { robot_up1, robot_up2, robot_up3, robot_up4 };
unsigned char *animframe_robot_down[4] = { robot_down1, robot_down2, robot_down3, robot_down4 };
unsigned char *animframe_robot_left[4] = { robot_left1, robot_left2, robot_left3, robot_left4 };
unsigned char *animframe_explosion[6] = { explode1, explode2, explode3, explode4, explode5, explode6 };


//extern char title[];				// title bmp
//extern char mp[];					// sequence
char mp[];
extern char maze[14][10];

// Functions from maze.c
extern void InitMaze(void);
extern void GenerateMaze(unsigned int seed);

// Lynx Sprite Control Block structure
typedef struct SCB {
	uchar sprctl0;
	uchar sprctl1;
	uchar sprcoll;			// spr collision number
	struct SCB *next;
	uchar *data;
	signed int hpos;
	signed int vpos; 
	uint hscale;
	uint vscale;
	uchar palmap[8];
	uint collResult;		// collision result data (offset = 23)
} TSCB;

// offset of collision result from beginning of SCB
#define COLLSCBOFFSET 23

extern struct SCB bgSCB;			// draws BG maze
extern struct SCB playerSCB;

//////////////////////////////////////////////////////////////////////////////
// SPRITE CONTROL BLOCKS
//////////////////////////////////////////////////////////////////////////////

//// Title SCB
//uint titleCOL = 0;
//struct SCB titleSCB	=	{	0xc0, 0x10, 0x20,			// compressed
//							NULL, title,
//							0x100, 0x100, 0x100, 0x100,
//							{ 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef },
//							0 };

// Clear screen sprite data and SCB
uchar sprclr[4] = { 0x03, 0x08, 0x00, 0x00 };

struct SCB clearSCB =	{	0xc0,			// 4bpp / bg_no_collision
							0x10,			// compressed, hsize/vsize
							0x00,			// 0 to clear coll buffer 
							&playerSCB, sprclr,
							0x100, 0x100, 0xa000, 0x6600,
							{ 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef },
							0 };

// "Background" (maze) SCB
struct SCB bgSCB =		{	0xc4,			// 4bpp, background shadow
							0x90,			// literal, scale XY
							0x01,			// collision number 1
							NULL, playfield,
							0x102, 0x100, 0x300, 0x300,
							{ 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef },
							0 };
// TEST
// "Background" (maze) SCB to draw directly to collision buffer
struct SCB collClearBGSCB =		{	0xc1,			// 4bpp, background non-collide
							0x90,			// literal, scale XY
							0x01,			// collision number comes from sprite pixel values
							NULL, playfield,
							0x102, 0x100, 0x300, 0x300,
							{ 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef },
							0 };
// END TEST

// Player SCB
struct SCB playerSCB =	{	0xc4,			// 4bpp, normal
							0x90,			// literal, scaleXY
							0x02,			// collision number 2
							NULL, player,	//pl_d1,
							0x140, 0x120, 0x100, 0x100,
							{ 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef },
							0 };

// Otto SCB
struct SCB ottoSCB =	{	0xc4,			// 4bpp,  normal collidable
							0x98,			// literal, use existing palette
							0x05,			// collision number 5
							NULL, otto1,
							0x120, 0x120, 0x100, 0x100,
							{ 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef },
							0 };

//// Title SCB
//struct SCB titleSCB =	{	0x44,			// 2bpp, background no collision
//							0x18,			// compressed, scaleXY, re-use palette
//							0x00,			// collision number 0
//							NULL, berzerk_title,	//pl_d1,
//							0x140, 0x120, 0x100, 0x100,
//							{ 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef },
//							0 };



// Robot SCB's
struct SCB robotSCB[MAX_ROBOTS];
// Bullet SCBs
struct SCB bulletSCB[MAX_BULLETS];
// Score SCBs
#define SCORESCBSIZE 10
struct SCB scoreSCB[SCORESCBSIZE];		// 6 numbers + 4 lives

// Set up robot SCBs
static void InitRobotSCBs()
{
	int i;
	for (i = 0; i < MAX_ROBOTS; i++)
		{
		robotSCB[i].sprctl0 = 0xc4;			// 4bpp, normal collidable
		robotSCB[i].sprctl1 = 0x90;			// literal
		robotSCB[i].sprcoll = 0x03;			// collision number 3
		robotSCB[i].next = &robotSCB[i + 1];
		if (i == MAX_ROBOTS - 1)
			robotSCB[i].next = NULL;
		robotSCB[i].data = robot_down1;
		robotSCB[i].hpos = 0x110;
		robotSCB[i].vpos = 0x40;
		robotSCB[i].hscale = 0x100;
		robotSCB[i].vscale = 0x100;
		memcpy(robotSCB[i].palmap, playerSCB.palmap, 8);
		robotSCB[i].collResult = 0;
		}
}

// Set up bullet SCBs
static void InitBulletSCBs()
{
	int i;
	for (i = 0; i < MAX_BULLETS; i++)
		{
		bulletSCB[i].sprctl0 = 0xc4;			// 4bpp,  normal collideable
		bulletSCB[i].sprctl1 = 0x90;			// literal sprite
		bulletSCB[i].sprcoll = 0x04;			// collision number 4
		bulletSCB[i].next = &bulletSCB[i + 1];
		if (i == MAX_BULLETS - 1)
			bulletSCB[i].next = NULL;
		bulletSCB[i].data = laser_lr; //head1;
		bulletSCB[i].hpos = 0x0;
		bulletSCB[i].vpos = 0x0;
		bulletSCB[i].hscale = 0x100;
		bulletSCB[i].vscale = 0x100;
		memcpy(bulletSCB[i].palmap, playerSCB.palmap, 8);
		bulletSCB[i].collResult = 0;
		}
}

char *digArray[10] = { dig0, dig1, dig2, dig3, dig4, dig5, dig6, dig7, dig8, dig9 };

// Set up score SCBs
static void InitScoreSCBs()
{
//;; score digits
//_SCBsc1		dc.b $c1,$10,$20
//            dc.w _SCBsc2,_dig0                        ; digit
//            dc.w $102,$160,$100,$100
//            dc.b $01, $23, $45, $67, $89,$Ab,$cd,$ef

	int i;
	for (i = 0; i < SCORESCBSIZE; i++)
		{
		scoreSCB[i].sprctl0 = 0xc1;			// 4bpp, background non-collideable
		if (i < 6)
			scoreSCB[i].sprctl1 = 0x18;			// compressed, use exist pal
		else
			scoreSCB[i].sprctl1 = 0x98;			// literal, use existing palette
		scoreSCB[i].sprcoll = 0x20;			// non-collidable
		scoreSCB[i].next = &scoreSCB[i + 1];
		if (i == SCORESCBSIZE - 1)
			scoreSCB[i].next = NULL;
		scoreSCB[i].data = sprclr; //dig1;
		scoreSCB[i].hpos = 0x120 + (i * 8);
		scoreSCB[i].vpos = 0x100 + 95;
		scoreSCB[i].hscale = 0x100;
		scoreSCB[i].vscale = 0x100;
		memcpy(scoreSCB[i].palmap, playerSCB.palmap, 8);
		scoreSCB[i].collResult = 0;
		}
}

#include "work.pal"				// work palette
#include "befok.pal"			// main game pal

#include "sin.h"				// sintable 0 -> 127 -> 0-> -127 -> 0
#include "volenv.h"				// volume envelopes
//#include "objseq.h"				// object sequencing
//#include "robot.c"
//#include "audio/smp_intruder.c"		// samples
//#include "audio/smp_alert.c"
//#include "audio/smp_chicken.c"
//#include "audio/smp_destroy.c"
//#include "audio/smp_humanoid.c"
//#include "audio/smp_plfire.c"
//#include "audio/smp_explode.c"
			
//#include "spr_pl_hit.c"

// Sprite for lives remaining (literal)
unsigned char life_spr[] = {  
	0x4,   0x3, 0x33, 0x0, 
	0x4,   0x3, 0x3, 0x0, 
	0x4,   0x33, 0x33, 0x30, 
	0x4,   0x0, 0x30, 0x0, 
	0x4,   0x3, 0x3, 0x0, 
	0x4,   0x33, 0x3, 0x30, 0x00
	};

#include "font.c"					// font[] raw 4-bit packed (for direct copy to VRAM)

// Wait until a button is pressed			
void WaitKey()
{
	while(SUZY.joystick != 0) ;
	while (SUZY.joystick == 0) ;
}

// Enemy state defines
#define STATE_SLEEP			1
#define STATE_MOVEX			2
#define STATE_MOVEY			3
#define STATE_MOVE			4
#define STATE_FIRING		5
#define STATE_HIT			10

/// Initialise an actor object
/// @param[in] i		Index of actor in obj array
/// @param[in] type		The object type index
/// @param[in] scbIndex	Sprite index of the object (in object SCB array)
/// @param[in] x0		Initial x coord
/// @param[in] y0		Initial y coord
void InitActor(uchar i, uchar type, uchar scbIndex, uint x0, uint y0)
{
	if (i >= NUM_OBJS)
		return;

	// initialise, then set default values
	obj[i].type = type;
	obj[i].scbIndex = scbIndex;
	obj[i].x = x0;
	obj[i].y = y0;
	obj[i].state = 0;
	obj[i].counter = 0;
	obj[i].d1 = 0;
	obj[i].d2 = 0;
	obj[i].d3 = 0;
	obj[i].frame = 0;
	obj[i].numframes = 1;
/*
	// type-specific init here
	switch(type)
		{
		case BULLET :
			obj[i].state = STATE_SLEEP;
			obj[i].counter = 200;
			break;
		case ROBOT :
			obj[i].state = STATE_SLEEP;
			obj[i].counter = 50 + (random() % 50);	// wait 50 cycles before moving
			break;
		case OTTO :
			obj[i].state = STATE_SLEEP;
			obj[i].counter = 600;	// wait 600 cycles before moving
			break;
		default :
			break;
		}
*/
}

// Start indices for robots and bullets in the obj array
#define ROBOTOBJINDEX 1
#define BULLETOBJINDEX (ROBOTOBJINDEX + MAX_ROBOTS)

/// Initialise all actors for a specific level
/// @param[in] level		The "level" we are initialising for
void InitActors(uchar level)
{
	// Fixed actor (object) allocation:
	// Index		Object name
	// 0			Otto
	// 1 to 8		Robots (up to 8)
	// 9 and 10		Player bullets (2)
	// 11 to 15		Enemy bullets (5)
	uchar i, n;
	uint tx, ty;
	uchar robotCells[15];		// 5 x 3 robot placement occupancy cells

	// Reset all actors to INACTIVE
	for (i = 0; i < NUM_OBJS; i++)
		InitActor(i, INACTIVE, 0, 0, 0);

	// Add Otto first so we can collide robots with him
	InitActor(0, OTTO, 0, 0x3C0, 0x4B0);
	obj[0].state = STATE_SLEEP;
	obj[0].counter = 600;	// wait 600 cycles before moving otto

	// Add up to 8 robots for this level
	// Do not place a robot in a doorway
	// Do not place a robot on top of another robot
	memset(robotCells, 0, 15);
	robotCells[2] = 1;
	robotCells[5] = 1;
	robotCells[9] = 1;
	robotCells[12] = 1;
	for (i = 0; i < levelData[level].numRobots; i++)
		{
		// Find an unoccupied cell
		n = 2;
		while (1 == robotCells[n])
			n = (random() % 15);

		robotCells[n] = 1;

		tx = (n % 5) * 120;
		ty = (n / 5) * 120;
		InitActor(ROBOTOBJINDEX + i, ROBOT, i, 0x450 + tx, 0x448 + ty);
		obj[ROBOTOBJINDEX + i].state = STATE_SLEEP;
		obj[ROBOTOBJINDEX + i].counter = 50 + (random() % 50);	// wait 50 cycles before moving
		}

	// Add 7 bullets (2 player and 5 enemy)
	for (i = 0; i < MAX_BULLETS; i++)
		{
		InitActor(BULLETOBJINDEX + i, BULLET, i, 0, 0);
		obj[BULLETOBJINDEX + i].state = STATE_SLEEP;
		obj[BULLETOBJINDEX + i].counter = 1;		// TO 
		}
}

/// Add a bullet object
/// @param enemy		If > 0, this is an enemy bullet. Else player bullet.
/// @param limit		The max number of enemy bullets allowed (as per the level)
/// @param x0			Initial x position
/// @param y0			Initial y position
/// @return				Index bullet obj in obj array (0 if not created)
uchar FireBullet(uchar enemy, uchar limit, uint x0, uint y0)
{
	uchar i, retval;
	uchar searchStart, searchEnd;
	struct ACTOR_TYPE *pObject;
	retval = 0;

	// Find place to add object
	searchStart = BULLETOBJINDEX;
	searchEnd = BULLETOBJINDEX + 1;
	if (enemy)
		{
		searchStart = BULLETOBJINDEX + 2;
		if (limit > 5) { limit = 5; }
		searchEnd = BULLETOBJINDEX + 2 + limit - 1;
		}

	for(i = searchStart; i <= searchEnd; i++)
		{
		pObject = &obj[i];
		if(INACTIVE == pObject->type)
			{
			// add object to list here
			// initialise, then set default values
			InitActor(i, BULLET, i - BULLETOBJINDEX, x0, y0);
			// BULLET-specific init here
			pObject->state = STATE_MOVE;
			pObject->counter = 200;
			retval = i;						// return index
			break;							// bomb out of loop 
			}
		}

	// if not succesful, return 0
	return retval;
}



/// Update all actor objects
/// @param level		Current game level
/// @param bulletLimit	Max. number of bullets robots can fire simultaneously
void UpdateActors(uchar level, uchar bulletLimit)
{
	int i, n, count;
	int dx, dy;
	int sx, sy;
	char *firedata;
	uchar *p;
	struct ACTOR_TYPE *pObject;
	char bulletSpeed;

	// update objects that are active
	count = 0;
	for(i = 0; i < NUM_OBJS; i++)
		{
		pObject = &obj[i];
		if(pObject->type > INACTIVE)					// object type > 0 (ie: not INACTIVE)
			{
			// common to all objects
			pObject->counter--;
			// do object-specific behaviour
			switch(pObject->type) {
				case BULLET :
					// Behaviour: move in a straight line, constant speed
					// switch off if counter expired
					if(0 == pObject->counter)
						pObject->type = INACTIVE;
					pObject->x += pObject->dx;
					pObject->y += pObject->dy;
					break;					
				case ROBOT :
					// Robot behaviour: seek human for a random time, then shoot, then sleep
// DONE - Robot sleep less at higher levels
// DONE - Robots fire multiple times (need new state STATE_FIRING)
// TODO - Robot fire more often	
					// do state-specific stuff
					switch(obj[i].state) {
						case STATE_SLEEP :		// sleep (ie: do nothing)
							pObject->d1 = 0;	// For animation
							// switch to next state if counter expires
							if(0 == pObject->counter)
								{
								pObject->state = STATE_MOVEX;
								pObject->counter = 60 + (random() % 40);	// seek for 60 to 100 cycles
								}
							break;
						case STATE_MOVEX :		// align with human horizontally
							if (pObject->x > pl_x + 8)
								{
								// Robot can detect walls (using getpixel)
								dx = (pObject->x >> 2) - 0x107;
								dy = (pObject->y >> 2) - 0x100;
								p = (uchar *)(BUFFER1) + ((dy * 160) + dx >> 1);
								if (0x11 == *p)
									{
									// Hit wall - switch to next state
									if (pObject->counter > 5)
										pObject->counter = 5;
									}
								else
									{
									pObject->x--;
									pObject->d1 = 1;
									//*p = 0xFF;  // for debugging
									}
								}
							else if (pObject->x < pl_x - 8)
								{
								// Robot can detect walls (using getpixel)
								dx = (pObject->x >> 2) - 0xF9;
								dy = (pObject->y >> 2) - 0x100;
								p = (uchar *)(BUFFER1) + ((dy * 160) + dx >> 1);
								if (0x11 == *p)
									{
									// Hit wall - switch to next state
									if (pObject->counter > 5)
										pObject->counter = 5;
									}
								else
									{
									pObject->x++;
									pObject->d1 = 2;
									//*p = 0xFF;
									}
								}
							// switch to next state if counter expires
							if(0 == pObject->counter)
								{
								pObject->state = STATE_MOVEY;
								pObject->counter = 60 + (random() % 40);	// seek for 60 to 100 cycles
								}
							break;
						case STATE_MOVEY :		// align with human vertically
							if(pObject->y > pl_y)
								{
								// Robot can detect walls (using getpixel)
								dx = (pObject->x >> 2) - 0x100;
								dy = (pObject->y >> 2) - 0x107;
								p = (uchar *)(BUFFER1) + ((dy * 160) + dx >> 1);
								if (0x11 == *p)
									{
									// Hit wall - switch to next state
									if (pObject->counter > 5)
										pObject->counter = 5;
									}
								else
									{
									pObject->y--;
									pObject->d1 = 3;
									//*p = 0xFF;
									}
								}
							else
								{
								// Robot can detect walls (using getpixel)
								dx = (pObject->x >> 2) - 0x100;
								dy = (pObject->y >> 2) - 0xF9;
								p = (uchar *)(BUFFER1) + ((dy * 160) + dx >> 1);
								if (0x11 == *p)
									{
									// Hit wall - switch to next state
									if (pObject->counter > 5)
										pObject->counter = 5;
									}
								else
									{
									pObject->y++;
									pObject->d1 = 4;
									//*p = 0xFF;
									}
								}
							// switch to next state if counter expires
							if(0 == pObject->counter)
								{
								pObject->state = STATE_FIRING;
								// Set counter to allow n bullets to be shot
								pObject->counter = levelData[level].numBullets * 16 + 1;
								}
							break;
						case STATE_FIRING :		// Stopped and shooting
// DONE - Limit robot bullets according to the level
// TODO - Set robot bullet speed according to level
							// Time to fire another bullet?
							if (5 == (pObject->counter & 0x0F))
								{
								pObject->d1 = 0;
								bulletSpeed = (char)levelData[level].bulletSpeed;
								// Find direction to human
								sx = 0; sy = 0;
								firedata = laser_lr;
								dx = pObject->x - pl_x;
								dy = pObject->y - pl_y;
								// Don't fire if too close
								if (abs(dx) > 20 || abs(dy) > 20)
									{
									// Shoot horiz, vert or diagonal?
									if (abs(dx) > 2 * abs(dy))
										{
										// Horizontal
										firedata = laser_lr;
										if (dx > 0)
											sx = -6; //-bulletSpeed;  // -8;
										else
											sx = 6; //bulletSpeed;  // 8;
										}
									else if (abs(dy) > 2 * abs(dx))
										{
										// Vertical
										firedata = laser_up;
										if (dy > 0)
											sy = -6; //-bulletSpeed;  //-8;
										else
											sy = 6; //bulletSpeed;  // 8;
										}
									else
										{
										// Diagonal
										if (dx > 0)
											{
											sx = -5;
											if (dy > 0)
												{
												firedata = laser_diag2;
												sy = -4;
												}
											else
												{
												firedata = laser_diag1;
												sy = 4;
												}
											}
										else
											{
											sx = 5;
											if (dy > 0)
												{
												firedata = laser_diag1;
												sy = -4;
												}
											else
												{
												firedata = laser_diag2;
												sy = 4;
												}
											}
										}

// DONE - dedicated actor object range for robot bullets (max 5 bullets)
// DONE - dedicated actor object range for player bullets (max 2 bullets)

									// Set up the new bullet object
									//n = FireBullet(1, bulletLimit, pObject->x + sx * 4, pObject->y + sy * 4);
									n = FireBullet(1, bulletLimit, pObject->x + sx, pObject->y + sy);
									if(n > 0)
										{
										obj[n].dx = sx;
										obj[n].dy = sy;
										obj[n].pdata = firedata;
										// Bullet must know which robot is it coming from, so
										// that we can do bullet-robot collision detection properly
										obj[n].d3 = i; 

										// Robot fire sfx uses same intr as player, just an octave lower
										instrZapper.octave = 6;
										StartSound(1, &instrZapper, 63, 0);
										}
									}
								}

							// Switch this robot to next state?
							if(0 == pObject->counter)
								{
								pObject->state = STATE_SLEEP;
								pObject->counter = 100 - (level * 5);	// sleep for 100 to 40 cycles
								}
							break;
						case STATE_HIT :		// hit
							// do nothing, switch off when counter expired
							if(0 == pObject->counter)
								pObject->type = INACTIVE;
							break;
					} // end switch
					break;
				case OTTO :
    				// Otto behaviour: grow, then do half-sin bounce in direction of player
					// if above player, move down, else move up
						
					// do state-specific stuff
					switch(pObject->state) {
						case STATE_SLEEP :		// grow otto
							// switch to next state if counter expires
							if (0 == pObject->counter) {
								pObject->state = STATE_MOVE;
								pObject->counter = 600;	// chase player for 600 cycles
								// play sample
								PlaySample(smp_destroy);
								WaitFrames(10);	// ~1 seconds
								PlaySample(smp_intruder);
							}
							break;
						case STATE_MOVE :		// chase player
							pObject->x++;
							// find direction to human
							if(pObject->y > (pl_y + 18))
								pObject->y--;
							else
								pObject->y++;
							// calculate bounce offset
							pObject->d1 = sin[(pObject->x << 2) % 128] >> 1;

							// Play bounce sound effect
							if (0 == pObject->d1)
								StartSound(2, &instrBounce, 63, 0);

							// switch to next state
							if(0 == pObject->counter) {
								pObject->type = INACTIVE;
							}
							break;
					}
					break;
			} // end switch
		} // end if active
	} // next object

}

/*
// init seq player
// NB: sequence length (ticks) must be set manually
void init_seq(void)
{
	uchar i;
	uint len, offset;

	// set track offsets
	offset = 8;
	seqlength = 0;
	for(i=0; i<4; i++) {
		track[i] = mp + offset;
		len = (uchar)mp[i*2];
		len += (uchar)mp[i*2+1] << 8;
		offset += len;
	}

	// set envelopes
	vestart[0] = env1;			// piano
	vestart[1] = env1;
	vestart[2] = env3;			// side-kick
	vestart[3] = env1;
	ve[0] = vestart[0];
	ve[1] = vestart[1];
	ve[2] = vestart[2];
	ve[3] = vestart[3];

	// set initial volumes
	for(i=0; i<4; i++) note_vol[i] = 0;
	
	// activate tracks
	trk_on[0] = 0;
	trk_on[1] = 1;
	trk_on[2] = 1;
	trk_on[3] = 0;

	// set sequence length (MANUAL!!!)
	seqlength = 0x0500;
	
	// init timer count
	seqtime = 0;
}
*/

/*
// poll/update sequence player
// NB: shift bits must be written before counter started.
// NB: all values reset to 0 at reset (eg: attenX)
// NB: at least one feedback tap must be on to get a sound.
void poll_seq(void)
{
	uchar i;
	uint trk_time, tvol;
	uchar *t;

	for(i=0; i<4; i++) {
		if(trk_on[i]) {
			trk_time = *(uint *)track[i];			// ouch!
			if(trk_time <= seqtime) {				// note on
				t = track[i];
				// reset vol env pntr to start of envelope
				ve[i] = vestart[i];
				// set freq and note vol for this channel
				channels[i].reload = t[3];
				channels[i].feedback = t[4];
				note_vol[i] = (t[2] & 0x3f);
				track[i] += 5;						// update pointer
			}

			// update volume from volume envelope
			// first check if this is a loop command
			if(*ve[i] & 0x80) ve[i] = vestart[i] + (*ve[i] - 128) * 2;	// optimise!
			tvol = note_vol[i] * ve[i][0];
			tvol = tvol >> 7;
			channels[i].volume = tvol & 0x3f;		// mult by 2!
			ve[i]++;
		}
	}
	// update counter and loop if neccessary
	seqtime++;
	if(seqtime > seqlength) init_seq();
}
*/


/*
// checkerboard fade
void DoCheckerboardFade(void) {
	uchar *i;
	uchar j;
	int k;

	i = MIKEY.scrbase;	//ScreenBuffer;

	for(j=0; j<4; j++) {
		i = (MIKEY.scrbase + j);
		for(k=0; k < 2040; k++) {
			*i = 0xFF;
			i += 4;
		}
	}
}
*/

// reset work pal to black
void reset_workpal(void) {
	uchar i;
	for(i = 0; i < 32; i++)
		work_palette[i] = 0x00;
}

// NEW replacement for SetRGB()
void SetPalette(char *pal)
{
	uchar i;
	for (i = 0; i < 32; i++)
		MIKEY.palette[i] = pal[i];
}

// Set a colour in the palette
/// @param index		Index of the colour in the palette
/// @param R			Red value (0 to 255)
/// @param G			Green value (0 to 255)
/// @param B			Blue value (0 to 255)
void SetColour(uchar index, uchar R, uchar G, uchar B)
{
	if (index < 16)
		{
		// Set green value
		MIKEY.palette[index] = (G >> 4); 
		MIKEY.palette[index + 16] = (B & 0xF0) | (R >> 4);
		}
}

/*
// fade work palette to
void fade_in(void) {
	uchar x, y, c, d;
	
	// fade in to required palette
	for(y=0; y<16; y++) {
		for(x=0; x<16 ; x++) {
			if(work_palette[x] < befok_palette[x]) work_palette[x]++;
			d = c = work_palette[x+16];
			c &= 0xF0;
			d &= 0x0F;
			if(c < (befok_palette[x+16] & 0xF0)) c += 0x10;
			if(d < (befok_palette[x+16] & 0x0F)) d += 0x01;
			work_palette[x+16] = c+d;
		}
		VSYNC(); VSYNC();
		SetPalette(work_palette);
	}
}

// fade out palette
void fade_out(void) {
	uchar x, y, c, d;

	// copy pal to workpal
	for(x=0; x<32; x++) {
		work_palette[x] = befok_palette[x];
	}
	
	// fade out palette
	for(y=15; y ; y--) {
		for(x=0; x<16 ; x++) {
			if(work_palette[x]) work_palette[x]--;
			d = c = work_palette[x+16];
			c &= 0xF0;
			d &= 0x0F;
			if(c) c -= 0x10;
			if(d) d -= 0x01;
			work_palette[x+16] = c+d;
		}
		VSYNC(); VSYNC();
		SetPalette(work_palette);
	}
}
*/

//// read joystick
//uchar joypad(void)
//{
//	//return PEEK(0xFCB0);
//	return SUZY.joystick;
//}

// Clear BG literal sprite
void bg_clear(void) {
	int x, y;

	for(y=0; y<31; y++) {
		for(x=1; x<27; x++) {
			playfield[y * 28 + x] = 0;
		}
	}
}

// Plot a rectangle on the BG literal sprite
void bg_rectangle(uchar x0, uchar y0, uchar w, uchar h, uchar colour)
{
	uchar hcount, y;
	int p;
	for (y = y0; y < y0 + h; y++)
		{
		p = y * 28 + x0 + 1;		// 2 pixels per byte
		for (hcount = 0; hcount < w; hcount++)
			{
			playfield[p + hcount] = colour;
			} 
		}
}

// Plot maze on BG literal sprite
void bg_plot(void) {
	int x, y;
	char c;
	int p;

/*
	for(x=0; x<14; x++) {
		for(y=0; y<10; y++) {
			c = maze[x][y];
			c |= (c << 4);
			// plot the pixel
			p = y*28 + x + 1;
			playfield[p] = c;
		}
	}
*/

	// draw horizontal lines
	for(x=0; x<5; x++) {
		for(y=0; y<10; y+=2) {
			c = maze[x*2+2][y+1];
			if(c == 0) {
				c = 0x11;
				// plot the line
				p = y*28*5 + x*5 + 1;
				playfield[p] = c; playfield[p+1] = c; playfield[p+2] = c;
				playfield[p+3] = c; playfield[p+4] = c; playfield[p+5] = c;
			}
		}
	}
	
	// draw vertical lines
	for(x=0; x<6; x++) {
		for(y=0; y<5; y+=2) {
			c = maze[x*2+1][y+2];
			if(c == 0) {
				c = 0x11;
				// plot the line
				p = y*28*5 + x*5 + 1;
				playfield[p] = c; playfield[p+28] = c; playfield[p+56] = c; playfield[p+84] = c;
				playfield[p+112] = c; playfield[p+140] = c; playfield[p+168] = c; playfield[p+196] = c;
				playfield[p+224] = c; playfield[p+252] = c;
			}
				
		}
	}

	// Draw exit blocks
	c = 0xDD;
	if (EXIT_LEFT == last_exit)
		bg_rectangle(25, 10, 1, 10, c);		// block rigth exit
	else if (EXIT_TOP == last_exit)
		bg_rectangle(11, 30, 4, 1, c);		// block bottom exit
	else if (EXIT_RIGHT == last_exit)
		bg_rectangle(0, 10, 1, 10, c);		// block left exit
	else if (EXIT_BOTTOM == last_exit)
		bg_rectangle(11, 0, 4, 1, c);		// block top exit
}

/*
// Vertical "blank" interrupt routine
//VBL() interrupt
void VBL()
{
	VBLflag = 0; // indicates that a VBL has ocurred
	lines = 0;				// hopefully optimised
	frames++;
}
*/

/*
// Horizontal "blank" interrupt routine
// note: get cycle count!
HBL() interrupt
{
#asm

ch_clr	pha
		phx
		ldx	_lines
		inc _lines
		inc _lines
		lda _bgcol,x
		sta	$FDA0
		lda _bgcol+1,x
		sta $FDB0
		plx
		pla
#endasm
}
*/

/*
// Test Horizontal "blank" routine
// (to measure no of scanlines taken to draw sprites)
//HBL() interrupt
void HBL()
{
//#asm
//ch_clr  pha
//        lda _bgtestcol
//        sta $FDA0
//        pla
//#endasm
asm("pha");
asm("lda %v", bgtestcol);
asm("sta $FDA0");
asm("pla");
}
*/

// DEBUG - for timing
void SetBGTestColour(uchar bgcol)
{
	MIKEY.palette[0] = bgcol;
}

// Replacement for tgi_busy()
void WaitSuzy()
{
//	while (tgi_busy()) ;

	while (SUZY.sprsys & 0x01)
		{
asm("nop");
asm("nop");
asm("nop");
asm("nop");
asm("nop");
asm("nop");
asm("nop");
asm("nop");
		}
}

/*
// print my font char directly to vram
void RawPrintChar(uchar x0, uchar y0, uchar c)
{
	uchar *p1;
	uchar *pfont;

	c -= 48;
	pfont = font + (c<<4) + (c<<1);
	p1 = RenderBuffer + x0 + y0 * 80;
	*p1++ = *pfont++;
	*p1++ = *pfont++;
	*p1++ = *pfont++;
	p1 += 77;
    *p1++ = *pfont++;
    *p1++ = *pfont++;
    *p1++ = *pfont++;
    p1 += 77;
    *p1++ = *pfont++;
    *p1++ = *pfont++;
    *p1++ = *pfont++;
    p1 += 77;
    *p1++ = *pfont++;
    *p1++ = *pfont++;
    *p1++ = *pfont++;
    p1 += 77;
    *p1++ = *pfont++;
    *p1++ = *pfont++;
    *p1++ = *pfont++;
}
*/

// print my font char directly to vram
void RawPrintString(uchar x0, uchar y0, char *s)
{
	uchar *p1;
	uchar *pfont;
	uchar c;

	p1 = RenderBuffer + x0 + y0 * 80;
	while(*s) {
		c = *s++ - 48;
		pfont = font + (c<<4) + (c<<1);
        // manually unrolled :)
		*p1++ = *pfont++;
		*p1++ = *pfont++;
		*p1++ = *pfont++;
		p1 += 77;
        *p1++ = *pfont++;
        *p1++ = *pfont++;
        *p1++ = *pfont++;
        p1 += 77;
        *p1++ = *pfont++;
        *p1++ = *pfont++;
        *p1++ = *pfont++;
        p1 += 77;
        *p1++ = *pfont++;
        *p1++ = *pfont++;
        *p1++ = *pfont++;
        p1 += 77;
        *p1++ = *pfont++;
        *p1++ = *pfont++;
        *p1++ = *pfont++;
        p1 += 77;
        *p1++ = *pfont++;
        *p1++ = *pfont++;
        *p1++ = *pfont++;
		p1 -= 400;
	}
}

// NEW (replacement for old lib)
void DrawSprite(struct SCB *startSCB)
{
	tgi_sprite(startSCB);

	// Wait for sprite engine to finish
	WaitSuzy();
}

// NEW (relacement for old lib)
void SwapBuffers()
{
	tgi_updatedisplay();
	RenderBuffer = MIKEY.scrbase;

/*
	// Wait for the next VBL, then
	// 1. Set view buffer to the draw buffer
	// 2. Swap draw buffer index (drawbuffer xor 1).
	VSYNC();

	// Set visible buffer (display buffer) to the one we have been drawing on
	// (According to Handy Specifications), MIKEY.DISPADDR is the backup or reload value
	// that gets transferred to the address counter at the start of the 3rd line of
	// vertical blanking)
	MIKEY.scrbase = RenderBuffer;

	// Swap double-buffer
	if (RenderBuffer == (uchar *)BUFFER1)
		RenderBuffer = (uchar *)BUFFER2;
	else
		RenderBuffer = (uchar *)BUFFER1;

	// Set Suzy draw buffer (base address of sprite video build buffer)
	//SUZY.sprbase = RenderBuffer;
	SUZY.vidadr = RenderBuffer;
*/
}

// Initialise Lynx, display and sound
void InitSystem()
{
	// tgi init
	tgi_install(tgi_static_stddrv);

	// SET FRAMERATE
	//tgi_setframerate(60);
//rate60: lda     #$9e            ; 60 Hz
//        ldx     #$29
//        sta     HTIMBKUP
//        stx     PBKUP
	MIKEY.timer0.reload = 0x9E;		// HTIMBKUP = timer 0 (HBL) "backup" reg
	MIKEY.pkbkup = 0x29;

	// TGI INIT (VBL and COLLISION stuff)
	tgi_init();
//INIT:
//; Enable interrupts for VBL
//        lda     #$80
//        tsb     VTIMCTLA
//; Set up collision buffer to $A058
//        lda     #$58
//        sta     COLLBASL
//        lda     #$A0
//        sta     COLLBASH
//; Put collision index before sprite data
//        lda     #$FF
//        sta     COLLOFFL
//        lda     #$FF
//        sta     COLLOFFH

	//_VBL_TIMER.control = _VBL_TIMER.control | 0x80;
	SUZY.colbase = (unsigned char *)COLLBUF;
	// set offset to sprite collision depository to 23 (relative to SCB)
	// and set collision buffer address
	SUZY.colloff = COLLSCBOFFSET;

	CLI();
	WaitSuzy();
	//tgi_clear();
	//tgi_setcolor(COLOR_GREEN);
	//tgi_outtextxy(0, 0, "Hello World");
//tgi_updatedisplay();

	frames = 0;

	// set address of 2 display buffers and collision buffer
	// (collision buffer address set below again 'cos this don't work)
	// tgi now does this
	// tgi locations
	// #define COLLBUF     0xA058
	// #define BUFFER1     0xC038
	// #define BUFFER2     0xE018

	// set offset to sprite collision depository to 23 (relative to SCB)
	// and set collision buffer address
	SUZY.colloff = COLLSCBOFFSET;

	// reset palette
	reset_workpal();
//	SwapBuffers();
}

// define this to use direct clear of collision buffer (slightly faster)
#define USE_DIRECTCOLLCLEAR

/*
// Test speed of screen/collision clear code
void DoClearTest()
{
	//char s[10];
	uchar *savedDrawBuffer;

	// from maze.c
	InitMaze();
	GenerateMaze(1);
	bg_clear();
	bg_plot();

	clearSCB.next = &bgSCB;
	bgSCB.next = NULL;

	while (1)
		{
		while (tgi_busy()) ;

		SetBGTestColour(0x0F);
#if defined(USE_DIRECTCOLLCLEAR)
		// Stage 1 - Draw BG sprite directly to collision buffer (as non-collideable sprite)
		collClearBGSCB.next = NULL;
		savedDrawBuffer = MIKEY.scrbase;
		MIKEY.scrbase = (uchar *)(COLLBUF);
		DrawSprite(&collClearBGSCB);
		// Stage 2 = Draw BG sprite to screen buffer (as non-collideable sprite)
		collClearBGSCB.next = NULL;
		MIKEY.scrbase = RenderBuffer; //savedDrawBuffer;
		DrawSprite(&bgSCB);	//&collClearBGSCB);
#else
		DrawSprite(&clearSCB);			// Should really just need to draw BG SCB to clear
#endif
		SetBGTestColour(0x00);

//		itoa(PEEK(COLLBUF + 7), s, 10);
//		RawPrintString(0, 90, s);
		//if (PEEK(COLLBUF + 3) > 0)
		//	POKE(savedDrawBuffer, 0xFF);

		SwapBuffers();
		}
}
*/

// Do the Title screen
void DoTitle(void)
{
/*
	uint vscale = 0x1;
	//uchar joystick;

	// display title
	titleSCB.hpos = 0x150;
	titleSCB.vpos = 0x128;
	titleSCB.hscale = 0x100;
	titleSCB.vscale = 0x1;

	clearSCB.next = &titleSCB;

	// scale title
	for (vscale = 4; vscale < 0x101; vscale += 4)
		{
		titleSCB.vscale = vscale;
		DrawSprite(&clearSCB);
		SwapBuffers();

		if (SUZY.joystick != 0)
			break;
		}


	if (0 == SUZY.joystick)
		{
		RawPrintString(18, 84, "BY JUM HIG 2018");
		SwapBuffers();
		WaitKey();
		}
*/
/*
	int i;
	clearSCB.next = &titleSCB;
	//titleSCB.vscale = 0x8;
	for (i = 0; i < 264; i++)
		{
		titleSCB.hpos = i;
		DrawSprite(&clearSCB);
		SwapBuffers();
		if (SUZY.joystick)
			i = 255;
		}
*/
	//for (i = 8; i < 513; i += 8)
	//	{
	//	titleSCB.vscale = i;
	//	DrawSprite(&clearSCB);
	//	SwapBuffers();
	//	}

	// Minimum title screen
	DrawSprite(&clearSCB);
	SwapBuffers();
	RawPrintString(17, 48, "BEZERKOIDS ALPHA");
	RawPrintString(18, 84, "BY JUM HIG 2018");
	WaitKey();

	clearSCB.next = &bgSCB;

	// fade out palette
	//fade_out();
}

/// Update the sprite chain from the game objects (robots, otto etc)
void UpdateSprites()
{
	uchar i;
	uchar scbIndex;
	//uint tx, ty;
	uchar frame;
	int flipOffset;
	struct ACTOR_TYPE *pActor;
	struct SCB *pSCB;

	pActor = &obj[0];
	scbIndex = 0;
	for(i = 0; i < NUM_OBJS; i++)
		{
		// only draw the object if active
		switch(pActor->type)
			{
			case INACTIVE :
				break;
			case BULLET :
				//scbIndex = pActor->scbIndex;
				pSCB = &bulletSCB[pActor->scbIndex];
				pSCB->hpos = pActor->x >> 2;
				pSCB->vpos = pActor->y >> 2;
				pSCB->data = pActor->pdata;
				break;
			case ROBOT :
				//scbIndex = pActor->scbIndex;
				pSCB = &robotSCB[pActor->scbIndex];
				// check if exploding
				if(pActor->state == 10)
					{
					// Draw explosion frame
					frame = pActor->counter >> 2;
					if (frame < 6)
						{
						pSCB->data = animframe_explosion[5 - frame];
						// Adjust for explosion sprite hotpoint
						pSCB->hpos = (pActor->x >> 2);
						pSCB->vpos = (pActor->y >> 2);
						}

					// hide robot/explosion at end of count
					if (1 == pActor->counter)
						pSCB->hpos = 0;
					}
				else
					{
					// Draw robot
					flipOffset = (2 == pActor->d1) ? 4 : -4;

					pSCB->hpos = (pActor->x >> 2) + flipOffset;
					pSCB->vpos = (pActor->y >> 2) - 5;
					pSCB->sprctl0 = 0xc4; // no horiz flip

					// Sort out animation
					frame = (pActor->counter >> 2) % 4;
					switch(pActor->d1)
						{
						case 0 :					// standing
							pSCB->data = animframe_robot[frame];
							break;
						case 1 :					// left
							pSCB->data = animframe_robot_left[frame];
							break;
						case 2 :					// right
							pSCB->data = animframe_robot_left[frame];
							pSCB->sprctl0 = 0xe4; // flip horizontally
							break;
						case 3 :					// up
							pSCB->data = animframe_robot_up[frame];
							break;
						case 4 :					// down
							pSCB->data = animframe_robot_down[frame];
							break;
						}
					}
				break;
			case OTTO :
				ottoSCB.hpos = pActor->x >> 2;
				ottoSCB.vpos = (pActor->y - pActor->d1) >> 2;
				if (pActor->d1 & 0xF8) ottoSCB.data = otto1;
				else ottoSCB.data = otto2;
				break;
			} // end switch object active
		pActor++;
		} // next object

}

/// Update the score and lives display
void UpdateScoreDisplay(int score, uchar lives)
{
// OPTIMISE - Don't update lives here
// OPTIMISE - Only update score display when neccessary (not every Vsync)

	// Set score digits in score SCB chain
	struct SCB *pSCB;
	char n;
	char s[10];
	itoa(score, s, 10);
	n = strlen(s);
	while (n--)
		scoreSCB[n].data = digArray[s[n] - 48];

	// Set lives in score SCB chain
	pSCB = &scoreSCB[6];
	for (n = 6; n < SCORESCBSIZE; n++)
		{
		if (n - 6 < lives)
			//scoreSCB[n].data = life_spr;
			pSCB->data = life_spr;			
		else
			{
			//scoreSCB[n].data = sprclr;
			//scoreSCB[n].vpos = 0;		// hide off screen
			pSCB->data = sprclr;
			pSCB->vpos = 0;		// hide off screen
			}
		pSCB++;
		}
}

/// Get difficulty level from the current score
uchar GetLevelFromScore(int score)
{
	uchar i, level;
	level = NUM_LEVELS - 1;
	for (i = 1; i < NUM_LEVELS; i++)
		{
		if (score < levelData[i].startScore)
			{
			level = i - 1;
			break;
			}
		}

	return level;
}

/// Calculate bonus if all robots killed before exiting level
uchar CalcBonus(uchar level)
{
	uchar i;
	uchar robotsLeft;
	uchar bonus;
	bonus = 0;
	robotsLeft = 0;
	for (i = 0; i < MAX_ROBOTS; i++)
		{
		if (ROBOT == obj[ROBOTOBJINDEX + i].type)
			robotsLeft++;
		}

	if (0 == robotsLeft)
		bonus = levelData[level].numRobots * 10;

	return bonus;
}

// *****************************************************************
// **************************   M A I N   **************************
// *****************************************************************
void main() {
	uchar i, j;
	uchar fire_prev;
	//uchar *headsprite[5];
	uchar player_hit;
	uchar level;
	uchar bulletLimit;

	// debug
	uchar fb, fcount;
	char s[20];
	uint tx, ty;
	int sx, sy;
	int score;
	int hiscore;
	uchar lives;
	unsigned int room_number;
	uchar bonus;
	
	int n;
	uchar *firedata;
	char flipOffset;

	struct SCB *pBulletSCB;

	SetBGTestColour(0);

	hiscore = 0;
	
	// set up display buffers and init interrupts
	InitSystem();

	// set palette to game pal
	SetPalette(befok_palette);

	//SmpInit(0,1);				// channel 0, timer 1
	//EnableIRQ(1);    /* use a macro defined in lynxlib.h */
	//SmpStart(starkick, 20);

	// set H_OFF, V_OFF for sprites to 256, 256
	SUZY.hoff = 0x100;
	SUZY.voff = 0x100;

//	EnableIRQ(0);

tgi_setcollisiondetection(1);


//DoClearTest();
//  test
	InitSound();

	// display title screen
// Main outer game loop here:
// Title -> Intro -> Game -> Hiscore 
new_game:

// For some reason DoTitle screws up the screen flipping
	//DoTitle();

	//DoCheckerboardFade();

	// Reset score, lives and player position
	score = 0;
	lives = 3;
	room_number = (32768 + 127);
	pl_y = 0x4B0; pl_x = 0x428;		// 4x coords

level_start:
	// Get current level
	level = GetLevelFromScore(score);
	// Set robot colour according to level
	SetColour(6, levelData[level].robotR, levelData[level].robotG, levelData[level].robotB);
	// Get bullet limit for the current level
	bulletLimit = 0;
	if (level < NUM_LEVELS)
		bulletLimit = levelData[level].numBullets;

	fb = 1;
	fcount = 0;

	// from maze.c
	InitMaze();
	GenerateMaze(room_number);
	bg_clear();
	bg_plot();

	// Set up robots and bullets SCB "arrays"
	InitRobotSCBs();
	InitBulletSCBs();
	InitScoreSCBs();

	// Set up sprite chain
	// (Determines draw order and thus collision order)
	clearSCB.next = &bgSCB;
	bgSCB.next = &robotSCB[0];
	robotSCB[MAX_ROBOTS - 1].next = &ottoSCB;
	ottoSCB.next = &playerSCB;
	playerSCB.next = &bulletSCB[0];
	bulletSCB[MAX_BULLETS - 1].next = &scoreSCB[0];

	// initialise game objects
	InitActors(level);

	playerSCB.hpos = (pl_x >> 2) + flipOffset;		// adjust for hflipping when moving left/right
	playerSCB.vpos = (pl_y >> 2) - 5;

    // play sample
	PlaySample(smp_intruder);
	WaitFrames(8);
	PlaySample(smp_alert);
	//WaitFrames(8);
	//PlaySample(smp_chicken);
	//WaitFrames(8);
	//PlaySample(smp_fight);
	//PlaySample(smp_like);
	//PlaySample(smp_aye);
	//PlaySample(smp_robot);
	//PlaySample(smp_destroy);
	//WaitFrames(8);
	//PlaySample(smp_humanoid);

	fire_prev = 0; sx = 8; sy = 0;
	exited = 0;
	player_hit = 0;
	while(0 == exited)
		{
		if(player_hit)
			{	// PLAYER HIT
			// play explode sample on channel 2 using IRQ 6
			if(1 == player_hit)
				{
				//SmpInit(2, 6);
				//EnableIRQ(6);
				//SmpStart(smp_explode,0);

				// PLayer hit sfx
				StartSound(0, &instrHit, 63, 0);
				}
			
			// flash skeleton, then go up in puff of smoke
			if(player_hit % 2)
				playerSCB.data = player_hit1;
			else
				playerSCB.data = player_hit2;

			if (player_hit > 30)
				{
				if (player_hit > 45)
					playerSCB.data = player_hit4;
				else
					playerSCB.data = player_hit3;
				pl_y--;
				playerSCB.vpos = (pl_y >> 2) - 6;
				}

			if(player_hit++ == 180) exited = 2;				// end after 3 secs
			if(player_hit == 60) playerSCB.hpos = 0;		// hide after 1 sec
			}
		else
			{
			// get joystick;
			j = SUZY.joystick; //joypad();

			// Default player to standing if not moving
			playerSCB.data = player;
			playerSCB.sprctl0 = 0xc4;			// disable hflip bit ($20)
			flipOffset = 0;

			if(j & JOYPAD_DOWN)
				{
				pl_y += 2;
				if(pl_y == 0x560)
					{
					exited = 1; last_exit = EXIT_BOTTOM; pl_y = 0x424;
					}
				//if ((pl_y >> 2) % 2) playerSCB.data = player_down1;
				//	else playerSCB.data = player_down2;
				playerSCB.data = animframe_player_down[(pl_y >> 2) % 6];
				}
			else if(j & JOYPAD_UP)
				{
				pl_y -= 2;
				if(pl_y == 0x400)
					{
					exited = 1; last_exit = EXIT_TOP; pl_y = 0x544;
					}
				//if((pl_y >> 2) % 2) playerSCB.data = player_up1;
				//	else playerSCB.data = player_up2;
				playerSCB.data = animframe_player_up[(pl_y >> 2) % 6];
				}

			if(j & JOYPAD_RIGHT)
				{
// TODO - Flip sprite for rigth direction!
				pl_x += 2;
				if(pl_x == 0x660)
					{
					exited = 1; last_exit = EXIT_RIGHT; pl_x = 0x42C;
					}
				//if((pl_x >> 2) % 2) playerSCB.data = player_left1;
				//	else playerSCB.data = player_left2;
				playerSCB.data = animframe_player_left[(pl_x >> 2) % 6];
				playerSCB.sprctl0 = 0xe4;			// enable hflip bit ($20)
				flipOffset = 8;
				}
			else if(j & JOYPAD_LEFT)
				{
				pl_x -= 2;
				if(pl_x == 0x400)
					{
					exited = 1; last_exit = EXIT_LEFT; pl_x = 0x632;
					}
				//if((pl_x >> 2) % 2) playerSCB.data = player_left1;
				//	else playerSCB.data = player_left2;
				playerSCB.data = animframe_player_left[(pl_x >> 2) % 6];
				}

			if (exited)
				{
				playerSCB.vpos = 0x400;		// putplayer offscreen
				}
			else
				{
				playerSCB.hpos = (pl_x >> 2) + flipOffset;		// adjust for hflipping when moving left/right
				playerSCB.vpos = (pl_y >> 2) - 5;
				}

			// preload fire directions
			switch(j & 0xF0)
				{
				case 0x80 :		// up
					sx = 0; sy = -8; firedata = laser_up;
					break;
				case 0x90 :		// up / right
					sx = 5; sy = -5; firedata = laser_diag1;
					break;
				case 0x10 :		// right
					sx = 8; sy = 0; firedata = laser_lr;
					break;
				case 0x50 :		// down / right
					sx = 5; sy = 5; firedata = laser_diag2;
					break;
				case 0x40 :		// down
					sx = 0; sy = 8; firedata = laser_up;
					break;
				case 0x60 :		// down / left
					sx = -5; sy = 5; firedata = laser_diag1;
					break;
				case 0x20 :		// left
					sx = -8; sy = 0; firedata = laser_lr;
					break;
				case 0xA0 :		// up / left
					sx = -5; sy = -5; firedata = laser_diag2;
					break;
				}

			// check fire
			if(fire_prev)
				fire_prev--;
			if(!fire_prev && (j & BUTTON_INNER))
				{
				// dirn to fire in is preloaded above
				// Get next inactive player bullet object
				// (MAX 2 PLAYER BULLETS AND 5 ROBOT BULLETS!)
				n = FireBullet(0, 2, pl_x + 13 + sx, pl_y + sy);
				if (n > 0)
					{
					obj[n].dx = sx;
					obj[n].dy = sy;
					obj[n].pdata = firedata;

					// set fire delay counter
					fire_prev = 15;

					// Player fire sfx uses same intr as robot, just an octave higher
					instrZapper.octave = 5;
					StartSound(0, &instrZapper, 63, 0);
					}
				} // end if fire
			}

		// move CPU controlled objects
		UpdateActors(level, bulletLimit);

        //val = 0;
		//scbIndex = 0;

		UpdateSprites();
//WaitKey();

		// Clear collision buffer
		playerSCB.collResult = 0;			// Is this neccessary?

        // Draw sprite chain
		// (Clear -> Maze BG - > Robots (8) Otto -> Player -> Bullets (7) -> Score)
		WaitSuzy();

		SwapBuffers();

//SetBGTestColour(0x08);
		DrawSprite(&clearSCB);			// Should really just need to draw BG SCB to clear
//SetBGTestColour(0x00);

		//itoa(robotSCB[0].collResult, s, 10);
		//itoa(playerSCB.collResult, s, 10);
		//itoa(level, s, 10);
		//RawPrintString(0, 0, s);

        // check for player collision with anything
        if(!player_hit)
			{
            if (playerSCB.collResult > 0)
				player_hit = 1;
			}

		// Check for robot collision with walls/robots
		// (Robot AI tries to avoid walls, but may still clip the wall end)
		for (i = 0; i < MAX_ROBOTS; i++)
			{
			if (robotSCB[i].collResult)
				{
				n = ROBOTOBJINDEX + i;
				if (obj[n].state != 10)
					{
					obj[n].state = 10;		// kill robot
					obj[n].counter = 23;	// die in half a sec
					}
				}
			}

		// Check for bullet collisions
		pBulletSCB = &bulletSCB[0];
		for (i = 0; i < MAX_BULLETS; i++)
			{
			if (pBulletSCB->collResult)
				{
				n = BULLETOBJINDEX + i;
				if (1 == pBulletSCB->collResult)
					{		// wall hit
					// Kill bullet, hide sprite
					obj[n].type = INACTIVE;
					pBulletSCB->hpos = 0;
					}
				else if (2 == pBulletSCB->collResult && i >= 2)
					{		// player hit (by enemy bullet)
					player_hit = 1;
					// Kill bullet, hide sprite
					obj[n].type = INACTIVE;
					pBulletSCB->hpos = 0;
					}
				else if (3 == pBulletSCB->collResult)
					{		// robot hit
					// find nearest robot
					for (j = ROBOTOBJINDEX; j < BULLETOBJINDEX; j++)
						{
						if (ROBOT == obj[j].type)
							{
							// x-distance must be < something
							if (obj[j].x > obj[n].x)
								tx = obj[j].x - obj[n].x;
							else
								tx = obj[n].x - obj[j].x;

							if (tx < 16)
								{
								// y-distance must be < something
								if (obj[j].y > obj[n].y)
									ty = obj[j].y - obj[n].y;
								else
									ty = obj[n].y - obj[j].y;

								if(ty < 16)
									{
									// Do not kill robot with it's own bullet!
									if (j != obj[n].d3)
										{
										// Kill bullet, hide sprite
										obj[n].type = INACTIVE;
										pBulletSCB->hpos = 0;

										// play explosion sound
										StartSound(2, &instrExplode, 63, 0);

										// kill robot (if not exploding already ) & update score
										if (obj[j].state != 10)
											{
											obj[j].state = 10;
											obj[j].counter = 23;
											// Wiki: Robots are worth 50 points each
											score += 50;
											}
										break;
										}
									}
								}
							}
						}
					}
				}
			pBulletSCB++;	// next bullet SCB
			}


//bgtestcol = 0x0F;		

//bgtestcol = 0x00;		

// DEBUG
/*
s[0] = (val % 10) + 48;
s[1] = 0;
TextOut(0x110, 0x15C, 13, 5, s);
*/

		// print score using my raw print method
		//RawPrintChar(20, 80, 'J');
		//score2 = 0x0005;
		//score1 = BCDAdd(&score1, &score2);
		//BCDToASCII(&score1, score);
//		RawPrintString(60, 96, score);
//TextOut(0x178, 0x15C, 13, 0, score);

		// Update score display
		// TODO - Split up so score done one 1 frame, lives updated on other frame
		UpdateScoreDisplay(score, lives);

		//VSYNC();
//		SwapBuffers();


/*
		// update sound
		poll_seq();
		// update object sequencer
		objseqslow--;
		if(!objseqslow) { poll_objseq(); objseqslow = 4; }
*/
		UpdateSound();

		fcount++;
	} // end while

	EndAllSound();

	// exited maze, do next maze?
	if (1 == exited)
		{
		//playerSCB.hpos = 0;
		//playerSCB.vpos = 0;

//		PlaySample(smp_alert, 2590);
		// Add bonus if all robots killed
		bonus = CalcBonus(level);
		if (bonus > 0)
			{
			score += bonus;
			UpdateScoreDisplay(score, lives);

			itoa(bonus, s, 10);
			RawPrintString(28, 72, "BONUS ");
			RawPrintString(46, 72, s);
			SwapBuffers();

			WaitFrames(150);	// ~2.5 seconds
			}
		else
			{
			PlaySample(smp_chicken);
			WaitFrames(60);	// ~1 seconds
			}

		// Update room number
		if (EXIT_LEFT == last_exit) room_number--;
		else if (EXIT_TOP == last_exit) room_number -= 256;
		else if (EXIT_RIGHT == last_exit) room_number++;
		else if (EXIT_BOTTOM == last_exit) room_number += 256;

		goto level_start;
		}
	else if (2 == exited)		// player killed?
		{
		// Reset player position if player died
		pl_y = 0x4B0; pl_x = 0x428;		// 4x coords

		if(lives--)
			goto level_start;
		}

	RawPrintString(30, 44, "GAME OVER");
	SwapBuffers();
	WaitKey();
	goto new_game;

}
