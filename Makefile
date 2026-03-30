CXX = gcc
LUA_INCDIR ?= /usr/include
CXXFLAGS = $(shell python3-config --includes) -I$(LUA_INCDIR)
LUA_LIBDIR ?= /usr/lib
LUA_VERSION ?= 5.4
LDFLAGS = -L$(LUA_LIBDIR) $(shell python3-config --ldflags)

PREFIX ?= /usr

CXX ?= gcc
CXXFLAGS += -O2 -fPIC -g -I./luapython/
LUA_VERSION ?= 5.4
LDFLAGS += -lm -ldl

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

# SOURCES = $(wildcard *.c)

OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET) loader

$(TARGET): $(OBJECTS)
	$(CXX) -shared -o $@ $^ $(LDFLAGS)

loader:
	$(CXX) $(CXXFLAGS) -c luapython/loader.c -o loader.o
	$(CXX) -shared -o loader.so loader.o $(LDFLAGS)

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

debug: $(TARGET)
	# $(CXX) $(CXXFLAGS) -c luapython/debug.c -o debug.o

clean:
	rm -rf luapython/*.o
	rm -f luapython.so

install: $(TARGET)
	mkdir -p $(INSTALL_LIBDIR)/luapython
	mkdir -p $(INSTALL_LUADIR)/luapython
	cp $(TARGET) $(INSTALL_LIBDIR)/luapython/core.so
	cp loader.so $(INSTALL_LIBDIR)/luapython/loader.so
	cp luapython/*.lua $(INSTALL_LUADIR)/luapython

uninstall:
	rm -rf $(INSTALL_LUADIR)/luapython
	rm -rf $(INSTALL_LIBDIR)/luapython

