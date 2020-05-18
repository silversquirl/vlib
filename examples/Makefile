CC = clang -std=c11 -pedantic
CFLAGS = -Wall -g $(shell sdl2-config --cflags)
LDFLAGS = -lm $(shell sdl2-config --libs)

TARGETS = poly

.PHONY: all clean
all: $(TARGETS)

clean:
	rm -f $(TARGETS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
