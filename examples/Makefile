CC = clang -std=c11 -pedantic
CFLAGS = -Wall -g $(shell sdl2-config --cflags)
LDFLAGS = -lm $(shell sdl2-config --libs)

TARGETS = circ poly circ2poly

.PHONY: all clean
all: $(TARGETS)

clean:
	rm -f $(TARGETS)

%: %.c collision_example.h ../v2col.h ../v2draw.h ../v2vec.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
