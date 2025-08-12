# LuaPython

LuaPython是一个将Python数据结构与Lua语言集成的项目，允许在Lua中使用Python的字符串、集合、字典等数据结构。

## 功能特性

### 1. 字符串操作
- 字符串连接 (`..`)
- 字符串长度 (`#`)
- 字符串比较 (`==`, `<`, `<=`)
- 字符串重复 (`*`)

### 2. 集合操作
- 集合长度 (`#`)
- 集合并集 (`+`)
- 集合交集 (`*`)
- 集合差集 (`-`)
- 集合对称差集 (`~`)
- 集合相等比较 (`==`)

### 3. 字典操作
- 字典长度 (`#`)
- 字典索引访问 (`dict[key]`)
- 字典索引赋值 (`dict[key] = value`)
- 字典合并 (`+`)
- 字典键交集 (`*`)
- 字典键差集 (`-`)
- 字典相等比较 (`==`)

## 编译要求

- C++17 编译器 (GCC 7+ 或 Clang 5+)
- Lua 5.1+ 开发库
- Python 3.13 开发库
- Make

## 编译安装

```bash
# 编译项目
make

# 运行测试
make test

# 安装到系统
sudo make install

# 卸载
sudo make uninstall
```

## 使用方法

### 基本用法

```lua
-- 加载LuaPython模块
local luapython = require("luapython")

-- 字符串操作
local str1 = "Hello"
local str2 = "World"
local result = str1 .. str2  -- "HelloWorld"
print(#str1)  -- 5
print(str1 * 3)  -- "HelloHelloHello"

-- 集合操作
local set1 = {1, 2, 3, 4, 5}
local set2 = {4, 5, 6, 7, 8}
local union = set1 + set2  -- 并集
local intersection = set1 * set2  -- 交集
local difference = set1 - set2  -- 差集

-- 字典操作
local dict1 = {a = 1, b = 2, c = 3}
local dict2 = {b = 20, c = 30, d = 4}
local merged = dict1 + dict2  -- 合并
print(dict1.a)  -- 1
dict1.e = 5  -- 赋值
```

### 与Python互操作

```lua
-- 导入Python模块
local numpy = luapython.import("numpy")

-- 创建Python数组
local arr = numpy.array({1, 2, 3, 4, 5})

-- 使用Python字符串
local py_str = numpy.char.array("Hello Python")

-- 使用Python集合
local py_set = set({1, 2, 3, 4, 5})

-- 使用Python字典
local py_dict = dict({a = 1, b = 2, c = 3})
```

## 操作符映射

| Lua操作符 | 字符串操作 | 集合操作 | 字典操作 |
|-----------|------------|----------|----------|
| `..` | 连接 | 不支持 | 不支持 |
| `#` | 长度 | 长度 | 长度 |
| `==` | 相等 | 相等 | 相等 |
| `<` | 小于 | 不支持 | 不支持 |
| `<=` | 小于等于 | 不支持 | 不支持 |
| `+` | 不支持 | 并集 | 合并 |
| `-` | 不支持 | 差集 | 键差集 |
| `*` | 重复 | 交集 | 键交集 |
| `~` | 不支持 | 对称差集 | 不支持 |
| `[]` | 不支持 | 不支持 | 索引访问 |
| `[]=` | 不支持 | 不支持 | 索引赋值 |

## 错误处理

所有操作都会进行类型检查，如果操作不支持或参数类型错误，会抛出Lua错误：

```lua
-- 错误示例
local str = "hello"
local num = 42
local result = str + num  -- 错误：字符串不支持加法操作
```

## 性能考虑

- Python对象在Lua中通过lightuserdata表示，内存管理由Python负责
- 频繁的类型转换可能影响性能，建议在循环中缓存转换结果
- 大型数据结构的操作建议在Python端完成

## 许可证

本项目采用MIT许可证。

## 贡献

欢迎提交Issue和Pull Request来改进这个项目。

## 更新日志

### v1.0.0
- 实现字符串基本操作
- 实现集合基本操作
- 实现字典基本操作
- 添加与Python的互操作功能
