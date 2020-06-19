CC = clang -pedantic
CFLAGS = -Wall -g
LDFLAGS = -lm -lpthread -lrt

TARGETS = channel hashset hashtable network

.PHONY: all clean
all: $(TARGETS)

clean:
	rm -f $(TARGETS)

%: %.c
	$(CC) -std=c99 $(CFLAGS) -o $@ $< $(LDFLAGS)
