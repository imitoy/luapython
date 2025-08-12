CXX = g++
CXXFLAGS = -Wall -Wextra -fPIC $(shell python3-config --includes) -g
LDFLAGS = -llua -lpython3.13 -ldl 

# 源文件
SOURCES = luapython.cpp boolean.cpp number.cpp string.cpp set.cpp dict.cpp list.cpp tuple.cpp module.cpp function.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# 目标文件
TARGET = luapython.so
MAIN = main

# 默认目标
all: $(TARGET) #$(MAIN)

# 编译共享库
$(TARGET): $(OBJECTS)
	$(CXX) -shared -o $@ $^ $(LDFLAGS)

# 编译主程序
$(MAIN): main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

# 编译目标文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	rm -f $(OBJECTS) $(TARGET) $(MAIN)

# 测试
test: $(TARGET)
	lua test_luapython.lua

# 安装
install: $(TARGET)
	cp $(TARGET) /usr/local/lib/
	cp luapython.hpp /usr/local/include/

# 卸载
uninstall:
	rm -f /usr/local/lib/$(TARGET)
	rm -f /usr/local/include/luapython.hpp

.PHONY: all clean test install uninstall
