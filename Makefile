PREFIX ?= /usr

# -Wall -Wextra 

PYTHON3 = python3.13

CXX = g++
CXXFLAGS = -shared -fPIC -g -I$(PREFIX)/include/lua5.4 $(shell python3-config --includes) -DPREFIX="\"$(PREFIX)\"" -DPYTHON_LIB="\"lib$(PYTHON3).so\""
LDFLAGS = -lm -ldl $(shell python3-config --ldflags) "-lpython3.13"

SOURCES = luapython.cpp boolean.cpp number.cpp string.cpp set.cpp dict.cpp list.cpp tuple.cpp module.cpp function.cpp class.cpp
OBJECTS = $(SOURCES:.cpp=.o)

TARGET = luapython.so

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -shared -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

install: $(TARGET)
	mkdir -p $(PREFIX)/local/lib/lua/5.4/luapython/
	cp $(TARGET) $(PREFIX)/local/lib/lua/5.4/
	cp convert_pre.lua $(PREFIX)/local/lib/lua/5.4/luapython/
	cp python_init.lua $(PREFIX)/local/lib/lua/5.4/luapython/
	cp import.lua $(PREFIX)/local/lib/lua/5.4/luapython/

uninstall:
	rm -rf $(PREFIX)/local/lib/lua/5.4/$(TARGET)
	rm -rf $(PREFIX)/local/lib/lua/5.4/luapython/

.PHONY: all clean install uninstall
