CXX = gcc
LUA_VERSION = 5.4
LDFLAGS = /usr/lib/liblua.so.5.4 /usr/lib/libpython3.so
LD_LIBRARY_PATH = /usr/lib /usr/lib/python3.14/

PREFIX ?= /usr

CXX ?= gcc
CXXFLAGS += -O2 -fPIC -g -I./luapython/
LUA_VERSION = 5.4
LDFLAGS += -lm -ldl

TARGET = luapython.so

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

all: $(TARGET) debug

$(TARGET): $(OBJECTS)
	$(CXX) -shared -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

debug: $(TARGET)
	$(CXX) $(CXXFLAGS) -c luapython/debug.c -o debug.o

clean:
	rm -rf luapython/*.o
	rm -f luapython.so

install: $(TARGET)
	mkdir -p $(PREFIX)/lib/lua/$(LUA_VERSION)/luapython
	mkdir -p $(PREFIX)/share/lua/$(LUA_VERSION)/luapython
	cp $(TARGET) $(PREFIX)/lib/lua/$(LUA_VERSION)/luapython/core.so
	cp luapython/*.lua $(PREFIX)/share/lua/$(LUA_VERSION)/luapython

uninstall:
	rm -rf $(PREFIX)/lib/lua/$(LUA_VERSION)/luapython
	rm -rf $(PREFIX)/share/lua/$(LUA_VERSION)/luapython

