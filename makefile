cflags := -Wall -Wextra -Wno-unknown-pragmas
libs := -lm -lexplain

roll: roll.c *.h
	gcc -Ofast $(cflags) $< $(libs) -o $@

debug: roll.c *.h
	gcc -g -DDEBUG -DTRACE $(cflags) $< $(libs) -o $@
