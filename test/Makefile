TESTS := $(patsubst %.c,test\:%,$(wildcard *.c))

CC := clang -std=c11 -pedantic
CFLAGS := -Wall -g -O0

.PHONY: test clean
test: $(TESTS)
clean:
	rm -rf .vtest_cache

.PHONY: test\:%
.SECONDARY: $(patsubst test\:%,.vtest_cache/%,$(TESTS))
test\:%: .vtest_cache/%
	@echo '[TEST]  ' $(patsubst test:%,%,$@)
	@./$<

.vtest_cache/%: %.c vtest.h ../%.h
	@echo '[CC]    ' $(patsubst %.c,%,$<)
	@mkdir -p .vtest_cache
	@$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
