headers := parse.h plotting.h prob.h ranges.h settings.h sim.h translate.h util.h
cflags := -Wall -Wextra -Wno-unknown-pragmas
libs := -lm -lexplain

roll: roll.c $(headers)
	gcc -Ofast $(cflags) $< $(libs) -o $@

debug: roll.c $(headers)
	gcc -g -DDEBUG -DTRACE $(cflags) $< $(libs) -o $@
