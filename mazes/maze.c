// Maze gen proggie
// JH 2001

// Algorithm:
// 1. Start at random
// 2. Make random path (non-crossing) until not possible
// 3. Start path from random already open passage
// 4. goto 2 until all passages done

// width and height of maze (must be multiple of 2)
#define MAZE_WIDTH   14
#define MAZE_HEIGHT  10

#define WALL	0
#define PATH	1

// array for maze "blocks"
int maze[MAZE_WIDTH][MAZE_HEIGHT];

// Initialise maze data
void InitMaze(void) {
	int i, j;

    // make all wall
	for(i=0; i<MAZE_WIDTH; i++) {
    	for(j=0; j<MAZE_HEIGHT; j++) {
        	maze[i][j] = WALL;
		}
	}
	// make path round perimeter
    for(i=0; i<(MAZE_WIDTH-1);i++) {
    	maze[i][0] = PATH;
        maze[i][MAZE_HEIGHT-2] = PATH;
	}
    for(j=0; j<(MAZE_HEIGHT-1);j++) {
    	maze[0][j] = PATH;
        maze[MAZE_WIDTH-2][j] = PATH;
	}
}

// display maze
void PrintMaze(void) {
	int i, j;

    printf("Maze:\n");
    for(i=0; i<MAZE_HEIGHT; i++) {
    	for(j=0; j<MAZE_WIDTH; j++) {
        	if(maze[j][i] == WALL) { putch('#'); putch('#'); }
            else { putch(' '); putch(' '); }
		}
        printf("\n");
	}
    printf("\n");
}

// Generate a new maze
int GenerateMaze(void) {
	int d, n, num_done, dir_tried, done;
    int total;
    int x, y;

    // seed random numbers
    srand(time(0));

	// pick initial starting point
   	x = 2 + (rand() % ((MAZE_WIDTH/2)-2)) * 2;
   	y = 2 + (rand() % ((MAZE_HEIGHT/2)-2)) * 2;
    total = ((MAZE_WIDTH/2)-2) * ((MAZE_HEIGHT/2)-2);
    num_done = 1;
    done = 0;

    maze[x][y] = PATH;
    while(!done) {
	   	// find a new direction to move in
        dir_tried = 0;
        while(dir_tried != 0xF) {
			d = rand()%4;
        	switch(d) {
        		case 0 :	// up?
            		if(maze[x][y-2] == WALL) {
						maze[x][y-1] = PATH;
                        maze[x][y-2] = PATH;
                        y -= 2;
				        dir_tried = 0; num_done++;
                    }
                    break;
				case 1 :	// down?
            		if(maze[x][y+2] == WALL) {
						maze[x][y+1] = PATH;
                        maze[x][y+2] = PATH;
                        y += 2;
				        dir_tried = 0; num_done++;
                    }
                	break;
				case 2 :	// left?
            		if(maze[x-2][y] == WALL) {
						maze[x-1][y] = PATH;
                        maze[x-2][y] = PATH;
                        x -= 2;
				        dir_tried = 0; num_done++;
                    }
                	break;
				case 3 :	// right?
            		if(maze[x+2][y] == WALL) {
						maze[x+1][y] = PATH;
                        maze[x+2][y] = PATH;
                        x += 2;
				        dir_tried = 0; num_done++;
                    }
                	break;
			} // end switch
            dir_tried |= (1 << d);				// update direction tried bitmask
			printf("dir_tried: %X    num_done: %d / %d\n", dir_tried, num_done, total);
//            PrintMaze();
//            if(getch() == 27) return 0;
		} // end while dir_tried

        // check if we are done, else find a new path
        if(num_done == total) {
        	done = 1;
        }
        else {
        	// find a new starting point on an existing path
            x = 1; y = 1;
            while(maze[x][y] == WALL) {
            	x = 2 + (rand() % ((MAZE_WIDTH/2)-2)) * 2;
            	y = 2 + (rand() % ((MAZE_HEIGHT/2)-2)) * 2;
			}
		}

        // debug
        //done = 1;
	}


}

int main(void) {
	InitMaze();
    PrintMaze();
    GenerateMaze();
    PrintMaze();
    printf("Press any key to exit...");
    getch();
}

