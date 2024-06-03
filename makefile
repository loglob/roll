CFLAGS += -MMD -march=native -Wall -Wextra -Wno-unknown-pragmas
libs := -lm

MAP_OBJ:=$(patsubst %.c,%.o,$(wildcard *.c))

roll: $(MAP_OBJ)
	gcc $(CFLAGS) $^ $(libs) -o $@

# .d files are generated by gcc to enable lazy compiles
-include $(wildcard *.d)

clean:
	rm -f *.d *.o