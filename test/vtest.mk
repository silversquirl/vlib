VTEST_DIR ?= .
VTEST_TESTS := $(patsubst $(VTEST_DIR)/%.c,test\:%,$(wildcard $(VTEST_DIR)/*.c))
VTEST_HEADER_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

VTEST_DEPS ?=
VTEST_CC ?= clang
VTEST_CC := $(CC) -std=c11
VTEST_CFLAGS := -g -O0 -I$(VTEST_HEADER_DIR)/ $(VTEST_CFLAGS)
VTEST_LDFLAGS ?=

.PHONY: test test\:clean
test: $(VTEST_TESTS)
test\:clean:
	rm -rf $(VTEST_DIR)/.vtest_cache

.PHONY: test\:%
.SECONDARY: $(patsubst test\:%,$(VTEST_DIR)/.vtest_cache/%,$(VTEST_TESTS))
test\:%: $(VTEST_DIR)/.vtest_cache/%
	@echo '[TEST]  ' $(patsubst test:%,%,$@)
	@./$<

$(VTEST_DIR)/.vtest_cache/%: $(VTEST_DIR)/%.c $(VTEST_HEADER_DIR)/vtest.h $(VTEST_DEPS)
	@echo '[CC]    ' $(patsubst %.c,%,$<)
	@mkdir -p $(VTEST_DIR)/.vtest_cache
	@$(VTEST_CC) $(VTEST_CFLAGS) -o $@ $< $(VTEST_LDFLAGS)
