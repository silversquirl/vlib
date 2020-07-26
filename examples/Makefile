ifeq "$(CC)" "cc"
CC = clang
endif

CC := $(CC) -pedantic
CFLAGS = -Wall -g
LDFLAGS = -lm -lpthread -lrt

TARGETS = channel coroutines network vgl_gol vgl_model vgl_image

.PHONY: all clean
all: $(TARGETS)

clean:
	rm -f $(TARGETS)

vgl_%: vgl_%.11.c glad/glad.c
	$(CC) -std=c11 $(CFLAGS) -o $@ $^ $(LDFLAGS) -lGL -ldl -lglfw

%: %.11.c
	$(CC) -std=c11 $(CFLAGS) -o $@ $< $(LDFLAGS)

%: %.99.c
	$(CC) -std=c99 $(CFLAGS) -o $@ $< $(LDFLAGS)
