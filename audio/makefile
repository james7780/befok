#
# makefile BEFOK samples
#

all:	smp_alert smp_chicken smp_destroy smp_detected smp_fight smp_humanoid \
	smp_intruder smp_robot smp_plfire smp_explode

smp_alert: 
	wav2lsf -o alert.lsf alert.wav
	bin2src -Fialert.lsf -Fosmp_alert.c -Lsmp_alert -C

smp_chicken: 
	wav2lsf -o chicken.lsf chicken.wav
	bin2src -Fichicken.lsf -Fosmp_chicken.c -Lsmp_chicken -C

smp_destroy: 
	wav2lsf -o destroy.lsf destroy.wav
	bin2src -Fidestroy.lsf -Fosmp_destroy.c -Lsmp_destroy -C

smp_detected: 
	wav2lsf -o detected.lsf detected.wav
	bin2src -Fidetected.lsf -Fosmp_detected.c -Lsmp_detected -C

smp_fight: 
	wav2lsf -o fight.lsf fight.wav
	bin2src -Fifight.lsf -Fosmp_fight.c -Lsmp_fight -C

smp_humanoid: 
	wav2lsf -o humanoid.lsf humanoid.wav
	bin2src -Fihumanoid.lsf -Fosmp_humanoid.c -Lsmp_humanoid -C

smp_intruder: 
	wav2lsf -o intruder.lsf intruder.wav
	bin2src -Fiintruder.lsf -Fosmp_intruder.c -Lsmp_intruder -C

smp_robot: 
	wav2lsf -o robot.lsf robot.wav
	bin2src -Firobot.lsf -Fosmp_robot.c -Lsmp_robot -C

smp_plfire: 
	wav2lsf -o plfire.lsf plfire.wav
	bin2src -Fiplfire.lsf -Fosmp_plfire.c -Lsmp_plfire -C

smp_explode: 
	wav2lsf -o explode.lsf explode.wav
	bin2src -Fiexplode.lsf -Fosmp_explode.c -Lsmp_explode -C
