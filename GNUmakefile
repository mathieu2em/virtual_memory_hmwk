BUILD_DIR=build
CC=gcc
CFLAGS=-g -std=gnu99 -Wall -pedantic -Isrc -I$(BUILD_DIR)
LDFLAGS=
BISON=bison
FLEX=flex

OS := $(shell uname)
ifeq ($(OS),Darwin)
TAR=gtar # gtar for MAC (brew install gnu-tar) for linux change to tar
else
TAR=tar
endif

OBJS = tlb.o parse.tab.o vmm.o pm.o pt.o tokens.o common.o

.PHONY: default all clean format client server test

default: all

all: $(BUILD_DIR)/vmm

$(BUILD_DIR)/vmm: $(patsubst %.o, $(BUILD_DIR)/%.o, $(OBJS))
	$(CC) $(LDFLAGS) -o $@ $(patsubst %.o,$(BUILD_DIR)/%.o, $(OBJS))

$(BUILD_DIR)/%.o: src/%.c
	@[ -d "$$(dirname "$@")" ] || mkdir -p "$$(dirname "$@")"
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/parse.tab.c $(BUILD_DIR)/parse.tab.h: src/parse.y
	@[ -d "$$(dirname "$@")" ] || mkdir -p "$$(dirname "$@")"
	cd $(BUILD_DIR); $(BISON) -d ../src/parse.y

$(BUILD_DIR)/tokens.c: src/tokens.l
	@[ -d "$$(dirname "$@")" ] || mkdir -p "$$(dirname "$@")"
	$(FLEX) -o $@ $<

run: all
	$(BUILD_DIR)/vmm tests/BACKING_STORE.txt <tests/command.in
	
vg: all
	valgrind --show-leak-kinds=all $(BUILD_DIR)/vmm tests/BACKING_STORE.txt <tests/command.in

clean:
	$(RM) -r $(BUILD_DIR) *.aux *.log

release:
	$(TAR) -czv -f tp3.tar.gz --transform 's|^|tp3/|' \
	    src *.tex *.pdf tests GNUmakefile