VTEST_TESTS := $(patsubst %.c,test\:%,$(wildcard *.c))

VTEST_CC ?= clang -std=c11 -pedantic
VTEST_CFLAGS := -Wall -g -O0 $(VTEST_CFLAGS)
VTEST_DIR ?= .
#VTEST_LDFLAGS :=
#VTEST_DEPS :=

.PHONY: test test\:clean
test: $(VTEST_TESTS)
test\:clean:
	rm -rf .vtest_cache

.PHONY: test\:%
.SECONDARY: $(patsubst test\:%,.vtest_cache/%,$(VTEST_TESTS))
test\:%: .vtest_cache/%
	@echo '[TEST]  ' $(patsubst test:%,%,$@)
	@./$<

.vtest_cache/%: %.c $(VTEST_DIR)/vtest.h $(VTEST_DEPS)
	@echo '[CC]    ' $(patsubst %.c,%,$<)
	@mkdir -p .vtest_cache
	@$(VTEST_CC) $(VTEST_CFLAGS) -o $@ $< $(VTEST_LDFLAGS)
