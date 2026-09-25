# Builds the real TimbreOS CLI (cli.c/float.c/printersHost.c/byteq.c) as a
# host binary, using Tea-host's TEA scheduler port (nanosleep()/
# gettimeofday() in place of hardware timers) instead of the hand-written
# Python reimplementation in tools/fake_target.py. See main.c's own
# comment for the protocol (stdin/stdout, byte at a time, same shape a
# real board's serial port speaks).
#
# cli.c and tea.h are symlinks into this directory (not copies): cli.c
# needs Tea-host's simpler tea.h instead of TimbreOS's real one (which
# pulls in clocks.h/dictionary.h - hardware-specific), and since C's
# quoted #include resolves relative to the including file's own
# directory first, the only way to redirect that without editing
# TimbreOS/cli.c itself is to make cli.c "live" somewhere that has the
# right tea.h next to it.
#
# wordlist.c/help.c/wordlist.txt are generated (not checked in - hence
# the .gitignore) from hostwords.txt, which pulls in the same
# WordLists/clibindings.txt + floatwords.txt every board build uses, with
# no board-specific hardware words. Regenerate after any change to those:
#   make wordlist

TIMBREOS := ../../..
TEAHOST := ../../../../Tea-host
PARSEWORDS := $(TIMBREOS)/WordLists/parsewords.py

CC := gcc
CFLAGS := -std=c11 -Wall -O2 -I . -I $(TIMBREOS) -I $(TEAHOST)
LDLIBS := -lm

SOURCES := main.c $(TEAHOST)/tea.c cli.c \
           $(TIMBREOS)/float.c $(TIMBREOS)/printersHost.c $(TIMBREOS)/byteq.c \
           wordlist.c help.c

BIN := tea_host_cli
BIN_DOUBLE := tea_host_cli_double # same CLI with 64 bit floats (float.c's pool)

.PHONY: all double wordlist clean

all: $(BIN)

double: $(BIN_DOUBLE)

$(BIN): $(SOURCES) cli.c tea.h wordlist.c
	$(CC) $(CFLAGS) $(SOURCES) $(LDLIBS) -o $@

$(BIN_DOUBLE): $(SOURCES) cli.c tea.h wordlist.c
	$(CC) $(CFLAGS) -DFLOAT_DOUBLE $(SOURCES) $(LDLIBS) -o $@

# Only regenerated when missing (parsewords.py's own "Updates triggered
# by ... not existing" check) or with `make wordlist` explicitly - a
# clibindings.txt/floatwords.txt edit elsewhere doesn't retrigger this
# automatically, matching every other board's own prebuild-step wiring.
wordlist.c: hostwords.txt
	python3 $(PARSEWORDS) hostwords.txt

wordlist:
	rm -f wordlist.c help.c wordlist.txt
	python3 $(PARSEWORDS) hostwords.txt

clean:
	rm -f $(BIN) $(BIN_DOUBLE)
	rm -rf $(BIN).dSYM
