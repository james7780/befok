#
# makefile for CC65 runtime library
# local bin path
# global macros
#
CC = cc65
CC65_TARGET = lynx
OP = xopt
AS = ra65
LD = link65 -m -v
AR = libr65
RM = del
CFLAGS = -t $(CC65_TARGET) 
LDFLAGS = -t $(CC65_TARGET) 
LIBS = lynx.olb pl.olb robot.olb laser.olb
#OBJECTS=pl_d1.obj pl_d2.obj pl_u1.obj pl_u2.obj \
#	pl_l1.obj pl_l2.obj pl_r1.obj pl_r2.obj \
#	robot_d1.obj robot_d2.obj \
#	head1.obj head2.obj head3.obj head4.obj head5.obj \
#	laser_up.obj laser_lr.obj laser_diag1.obj laser_diag2.obj \
#	otto1.obj otto2.obj \
#	gold1.obj gold2.obj gold3.obj gold4.obj \

OBJECTS=otto1.obj otto2.obj \
	gold1.obj gold2.obj gold3.obj gold4.obj \
	test.obj \
	title.obj \
	maze.obj

.SUFFIXES: .com .ttp .o .obj .m65 .c
.c.obj:
	$(CC) $(CFLAGS) $<
	$(OP) $*.m65
	$(AS) $*.m65
#	$(RM) $*.m65
.m65.obj:
	$(AS) $<

all:	befok.com

befok.com : befok.obj $(OBJECTS) 
	$(LD) -b200 -s512 -o $*.com $*.obj $(LIBS) $(OBJECTS)

befok.obj : befok.c actors.h
maze.obj : maze.c

clean :
	$(RM) befok.com
	$(RM) befok.obj
	$(RM) befok.m65
#	$(RM) *.obj
#	$(RM) *.pal
#	$(RM) *.spr
	$(RM) *.bak
	$(RM) *.map
