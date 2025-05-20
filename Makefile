#CFLAGS += -Wall -Wextra -Werror
CFLAGS += -O3 -Wno-shift-op-parentheses

# eg for clang
WASM_CFLAGS += $(CFLAGS) --target=wasm32 -Os -flto -DN9_NOMAIN
WASM_LDFLAGS += --no-standard-libraries -Wl,--no-entry -Wl,--export-dynamic -Wl,--allow-undefined

PYTHON = python

TEST_SRCS := $(wildcard test_*.c)
TESTS := $(TEST_SRCS:.c=)


nether9:
all: nether9 index.html
check: all $(TESTS:=.run)

clean:
	$(RM) nether9 nether9.wasm index.html $(TESTS)

%:: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $< -o $@

%.wasm: %.c
	$(CC) $(WASM_CFLAGS) $(WASM_LDFLAGS) $^ -o $@

%.html: %.html.tmpl nether9.wasm
	$(PYTHON) embed.py $^ > $@

%.run: % FORCE
	./$(@:.run=)

$(TESTS): nether9.c

.DELETE_ON_ERROR:
.SECONDARY: nether9.wasm $(TESTS)
.PHONY: FORCE all check clean
FORCE:
