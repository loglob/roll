cflags := -Wall -Wextra -Wno-unknown-pragmas
libs := -lm

roll: roll.c *.h
	gcc -O2 $(cflags) $< $(libs) -o $@

debug: roll.c *.h
	gcc -g -DDEBUG -DTRACE $(cflags) $< $(libs) -o $@
