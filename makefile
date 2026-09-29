#
# FILE            makefile
#
# AUTHOR          Ken Zangelin
#
# Copyright 2026 Seamware
# SPDX-License-Identifier: Apache-2.0
#
#
# corJson is JSON and nothing else: the parser that produces a corTree, and the
# renderers that consume one. The tree itself - the node, building, lookup,
# clone, sort - is corTree. Also builds the `corJson` tool (what corTest pipes
# every response through, as `corJson -sort`).
#
# Every library in this stack is a SIBLING repo - `-I..` and `../<name>/lib<name>.a`
# is the layout, and it is part of the build contract rather than a convenience.
#
LIB_SO        = libcorJson.so
LIB           = libcorJson.a
CC            = gcc
INCLUDE       = -I..
DFLAGS        =
#
# EXTRA_CFLAGS - the hook for a caller that needs to ADD flags to this build.
# Not DFLAGS: `make DFLAGS=...` REPLACES it, and a `DFLAGS +=` here would be
# ignored along with it, so a caller adding one flag would drop every default.
#
CFLAGS        = -std=c11 -O2 -Wall -Wextra -Werror -fPIC -fstack-protector-strong $(DFLAGS) $(INCLUDE) -MMD -MP $(EXTRA_CFLAGS)

LIB_SOURCES   = CorJsonStatus.c      \
                corJsonConfig.c      \
                corJsonCreate.c      \
                corJsonParse.c       \
                corJsonRender.c      \
                corJsonRenderSize.c  \
                corJsonReset.c       \
                corJsonSax.c         \
                corJsonVersion.c

BUILD        ?= debug

#
# Traces (COR_T, COR_LIB_T) are compiled in for a debug build only - see corLog.h / corLibLog.h.
#
ifeq ($(BUILD),debug)
CFLAGS       += -DCOR_T_ON
endif
OBJDIR        = obj/$(BUILD)
OBJECTS       = $(LIB_SOURCES:%.c=$(OBJDIR)/%.o)
DEPS          = $(OBJECTS:.o=.d)

#
# The libraries this one and its tool link against, by path rather than by -L/-l:
# the sibling checkout is the source of truth, and a -l would happily find an
# older copy installed somewhere on the system.
#
LIBS          = ../corTree/libcorTree.a ../corAlloc/libcorAlloc.a ../corLog/libcorLog.a ../corBase/libcorBase.a -lpthread -lrt -lm

TOOL          = corJson

all: $(LIB) $(LIB_SO) $(TOOL)

#
# corJson - the command line tool: parse, validate, sort and re-render JSON
#
$(TOOL): corJsonTool.c $(LIB)
	$(CC) $(CFLAGS) -o $@ corJsonTool.c $(LIB) $(LIBS)

#
# $(OBJDIR)/.flags - rebuild when the COMPILE LINE changes
#
# A flag change is invisible to every timestamp: the sources are older than the
# objects and make sees nothing to do, so the build silently keeps objects
# compiled with the previous flags. This records them and makes the objects
# depend on the record.
#
$(OBJDIR)/.flags: FORCE
	@mkdir -p $(OBJDIR)
	@echo '$(CFLAGS)' | cmp -s - $@ || echo '$(CFLAGS)' > $@

$(OBJDIR)/%.o: %.c $(OBJDIR)/.flags
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

#
# Removed first: `ar r` replaces and adds but never removes, so an object that
# is no longer built stays in the archive forever, and the next link quietly
# uses code that is not in the tree any more.
#
$(LIB): $(OBJECTS)
	@rm -f $@
	ar rcs $@ $(OBJECTS)

$(LIB_SO): $(OBJECTS)
	$(CC) -shared -o $@ $(OBJECTS)

#
# install - NOT a copy into /usr/local. Consumers compile with `-I..` and link
# `../corJson/libcorJson.a` straight out of the checkout; what is left is the
# sibling convention - the tool goes into bin/, where corLibs picks it up.
#
install: all
	@if [ ! -d bin ]; then mkdir bin; fi
	cp $(TOOL) bin/

di: all install

ci: clean install

clean:
	rm -rf obj $(LIB) $(LIB_SO) $(TOOL) *.o *.d *.gcno *.gcda

FORCE:

.PHONY: all install di ci clean FORCE

-include $(DEPS)
