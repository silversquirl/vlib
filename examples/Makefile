CC = clang -pedantic
CFLAGS = -Wall -g
LDFLAGS = -lm -lpthread -lrt

TARGETS = channel hashset hashtable network vgl_gol

.PHONY: all clean
all: $(TARGETS)

clean:
	rm -f $(TARGETS)

vgl_gol: vgl_gol.11.c
	$(CC) -std=c11 $(CFLAGS) -o $@ $< $(LDFLAGS) -lGL -lGLEW -lglfw

%: %.11.c
	$(CC) -std=c11 $(CFLAGS) -o $@ $< $(LDFLAGS)

%: %.99.c
	$(CC) -std=c99 $(CFLAGS) -o $@ $< $(LDFLAGS)
