CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic -Werror -g
CPPFLAGS ?= -Isrc/sim -Itests

TEST_BINARY = build/proclens_tests
TEST_SOURCES = src/ds/structures.c src/sim/model.c src/sim/simulation.c src/analysis/report.c src/analysis/groups.c src/analysis/adaptive.c src/analysis/advisor.c src/collector/snapshot.c src/collector/collector_win.c src/collector/collector_mac.c tests/test_main.c

.PHONY: all test clean

all: test

CLI_SOURCES = $(filter-out tests/test_main.c,$(TEST_SOURCES)) src/cli/main.c
SERVER_SOURCES = $(filter-out tests/test_main.c,$(TEST_SOURCES)) src/server/http_runtime.c src/server/api.c src/server/main.c

ifeq ($(OS),Windows_NT)
SERVER_LIBS = -lws2_32
endif

cli: build
	$(CC) $(CFLAGS) $(CPPFLAGS) -Isrc/ds -Isrc/analysis -Isrc/collector $(CLI_SOURCES) -o build/proclens_cli

server: build
	$(CC) $(CFLAGS) $(CPPFLAGS) -Isrc/ds -Isrc/analysis -Isrc/collector $(SERVER_SOURCES) -o build/proclens_server $(SERVER_LIBS)

build:
	@mkdir -p build

$(TEST_BINARY): $(TEST_SOURCES) | build
	$(CC) $(CFLAGS) $(CPPFLAGS) -Isrc/ds -Isrc/analysis -Isrc/collector $(TEST_SOURCES) -o $(TEST_BINARY)

test: $(TEST_BINARY)
	./$(TEST_BINARY)
	node --check ui/app.js
	node tests/ui_render.mjs

clean:
	rm -rf build
