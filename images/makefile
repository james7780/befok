#
# makefile BEFOK sprites
#

all:	pl_hit robot_hit

pl_hit: 
	sprpck -u -t1 -a003003 -i008009 pl_hit.raw
	bin2src -Fipl_hit.spr -Fopl_hit.c -Lpl_hit -C

robot_hit: 
	sprpck -v -c -t1 -a003003 -i008009 robot_hit.raw
	bin2src -Firobot_hit.spr -Forobot_hit.c -Lrobot_hit -C
