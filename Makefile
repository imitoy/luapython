PREFIX = /usr
CXX = gcc
LUA_VERSION = 5.4
LUA_VERSION_A = 5.4
LDFLAGS =  /usr/lib/liblua.so.5.4 /usr/lib/libpython3.so
LD_LIBRARY_PATH=/usr/lib

PREFIX ?= /usr

LUA_VERSION ?= 5.4

CXX ?= gcc
CXXFLAGS = -shared -fPIC -g -I$(PREFIX)/include/lua$(LUA_VERSION) $(shell python3-config --includes) -DPREFIX="\"$(PREFIX)\"" -DPYTHON_LIB="\"libpython3.so\""
LDFLAGS += -lm -ldl

SOURCES = luapython.c number.c string.c set.c dict.c list.c tuple.c module.c function.c class.c
OBJECTS = $(SOURCES:.c=.o)

TARGET = luapython.so

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -shared -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

install: $(TARGET)
	mkdir -p $(PREFIX)/local/lib/lua/$(LUA_VERSION)/luapython/
	cp $(TARGET) $(PREFIX)/local/lib/lua/$(LUA_VERSION)/
	cp convert_pre.lua $(PREFIX)/local/lib/lua/$(LUA_VERSION)/luapython/
	cp python_init.lua $(PREFIX)/local/lib/lua/$(LUA_VERSION)/luapython/
	cp python_function.lua $(PREFIX)/local/lib/lua/$(LUA_VERSION)/luapython/
	cp import.lua $(PREFIX)/local/lib/lua/$(LUA_VERSION)/luapython/

uninstall:
	rm -rf $(PREFIX)/local/lib/lua/$(LUA_VERSION)/$(TARGET)
	rm -rf $(PREFIX)/local/lib/lua/$(LUA_VERSION)/luapython/

.PHONY: all clean install uninstall
