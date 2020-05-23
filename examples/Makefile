CC = clang -std=c99 -pedantic
CFLAGS = -Wall -g $(shell sdl2-config --cflags)
LDFLAGS = -lm $(shell sdl2-config --libs)

TARGETS = circ poly circ2poly ray2circ ray2poly close box

.PHONY: all clean
all: $(TARGETS)

clean:
	rm -f $(TARGETS)

%: %.c collision_example.h ../v2.h ../v2draw.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
