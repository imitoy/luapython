CC ?= gcc
LUA ?= lua
LUA_INCDIR ?= /usr/include
LUA_LIBDIR ?= /usr/lib
LUA_VERSION ?= 5.4
PREFIX ?= /usr

PYTHON_CFLAGS ?= $(shell python3-config --includes)
PYTHON_LDFLAGS ?= $(shell python3-config --ldflags)

# Required build flags live in their own variable so that a caller overriding
# CFLAGS (luarocks does that) cannot drop -fPIC and break the shared object.
MYCFLAGS = -I$(LUA_INCDIR) $(PYTHON_CFLAGS) -I./luapython/ \
           -fPIC -g -O2 -std=gnu99 \
           -Wall -Wextra -Wno-unused-parameter
MYLDFLAGS = -L$(LUA_LIBDIR) $(PYTHON_LDFLAGS) -lm -ldl -shared

override CFLAGS += $(MYCFLAGS)
override LDFLAGS += $(MYLDFLAGS)

TARGET = luapython.so

INSTALL_LIBDIR ?= $(PREFIX)/lib/lua/$(LUA_VERSION)
INSTALL_LUADIR ?= $(PREFIX)/share/lua/$(LUA_VERSION)

SOURCES = \
    luapython/luapython.c \
    luapython/number.c \
    luapython/string.c \
    luapython/set.c \
    luapython/dict.c \
    luapython/list.c \
    luapython/tuple.c \
    luapython/module.c \
    luapython/function.c \
    luapython/class.c \
    luapython/iter.c \
    luapython/tools.c

OBJECTS = $(SOURCES:.c=.o)

# Objects are ABI-specific: a .o compiled against Lua 5.3 headers references
# lua_newuserdata(), which Lua 5.4+ no longer exports. Reusing it across
# versions produces `undefined symbol` at dlopen time with no compiler
# warning, so record the ABI-relevant flags and force a rebuild when they
# change.
BUILD_FLAGS_STAMP = .build-flags
BUILD_FLAGS_VALUE = $(LUA_INCDIR)|$(LUA_VERSION)|$(PYTHON_CFLAGS)|$(CC)

all: $(TARGET) loader.so
	@echo CFLAGS: $(CFLAGS)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -shared -o $@ $^

# The loader itself must never depend on Python: it has to be loadable before
# any libpython is dlopen()ed, so it only gets the Lua include path.
loader.so: luapython/loader.c $(BUILD_FLAGS_STAMP)
	$(CC) -I$(LUA_INCDIR) -fPIC -g -O2 -std=gnu99 -Wall $(LDFLAGS) -shared -o $@ $<

$(OBJECTS): $(BUILD_FLAGS_STAMP)

$(BUILD_FLAGS_STAMP): FORCE
	@printf '%s\n' '$(BUILD_FLAGS_VALUE)' > $@.tmp
	@if cmp -s $@.tmp $@; then rm -f $@.tmp; else mv $@.tmp $@; fi

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf luapython/*.o
	rm -f *.so
	rm -f *.o
	rm -f $(BUILD_FLAGS_STAMP) $(BUILD_FLAGS_STAMP).tmp

install:
	mkdir -p $(INSTALL_LIBDIR)/luapython
	mkdir -p $(INSTALL_LUADIR)/luapython
	cp $(TARGET) $(INSTALL_LIBDIR)/luapython/core.so
	cp loader.so $(INSTALL_LIBDIR)/luapython/loader.so
	cp luapython/*.lua $(INSTALL_LUADIR)/luapython

uninstall:
	rm -rf $(INSTALL_LUADIR)/luapython
	rm -rf $(INSTALL_LIBDIR)/luapython

# Install into a throw-away tree and run the regression tests against it.
TESTDIR = build/test-tree
VALGRIND ?= valgrind
TEST_ENV = LUA_PATH="$(TESTDIR)/?.lua;$(TESTDIR)/?/init.lua;;" \
           LUA_CPATH="$(TESTDIR)/?.so;;"

stage: all
	@mkdir -p $(TESTDIR)/luapython
	@cp $(TARGET) $(TESTDIR)/luapython/core.so
	@cp loader.so $(TESTDIR)/luapython/loader.so
	@cp luapython/*.lua $(TESTDIR)/luapython/

test: stage
	$(TEST_ENV) $(LUA) test/test.lua

# Memory-error check. Leak checking stays off on purpose: CPython keeps its
# interpreter state alive until Py_Finalize(), and the suite calls finalize()
# only in its very last section. What matters here is invalid reads/writes.
valgrind: stage
	$(TEST_ENV) PYTHONMALLOC=malloc $(VALGRIND) --error-exitcode=42 \
		--leak-check=no --track-origins=yes $(LUA) test/test.lua

.PHONY: all clean install uninstall test stage valgrind FORCE

FORCE:
