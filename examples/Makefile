CC = clang -pedantic
CFLAGS = -Wall -g
LDFLAGS = -lm -lpthread -lrt

TARGETS = channel network vgl_gol vgl_model

.PHONY: all clean
all: $(TARGETS)

clean:
	rm -f $(TARGETS)

vgl_%: vgl_%.11.c
	$(CC) -std=c11 $(CFLAGS) -o $@ $< $(LDFLAGS) -lGL -lGLEW -lglfw

%: %.11.c
	$(CC) -std=c11 $(CFLAGS) -o $@ $< $(LDFLAGS)

%: %.99.c
	$(CC) -std=c99 $(CFLAGS) -o $@ $< $(LDFLAGS)
