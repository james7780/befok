// actors.h
// defines for BEZERK objects and their behaviours

// 1. Object engine
// 2. Level sequencing engine


// defines for game OBJECT types
#define INACTIVE      0
#define ROBOT         1
#define BULLET        2
#define OTTO          3

// game object struct
struct ACTOR_TYPE {
	uchar	type;
	uchar	scbIndex;	// Index into scb list for this type of object  
	uint	x, y;		// fix-pt?
	int		dx, dy;		// fix-pt?
	uint	d1, d2, d3;	// general use
	uchar	state;
	uint	counter;
	uchar	frame;		// current animation frames
	uchar	numframes;	// number of animation frames
	uchar	collision;	// shadow of collision value
	char 	*pdata;		// pointer to object data
};



