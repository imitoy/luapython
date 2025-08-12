-- 测试LuaPython字符串、集合和字典功能

print("=== 测试字符串操作 ===")

-- 测试字符串连接
local str1 = "Hello"
local str2 = "World"
local result = str1 .. str2
print("字符串连接:", result)

-- 测试字符串长度
print("字符串长度:", #str1)

-- 测试字符串比较
print("字符串相等:", str1 == "Hello")
print("字符串小于:", str1 < str2)

-- 测试字符串重复
local repeated = str1 * 3
print("字符串重复:", repeated)

print("\n=== 测试集合操作 ===")

-- 创建集合
local set1 = {1, 2, 3, 4, 5}
local set2 = {4, 5, 6, 7, 8}

-- 测试集合长度
print("集合1长度:", #set1)
print("集合2长度:", #set2)

-- 测试集合并集
local union = set1 + set2
print("集合并集长度:", #union)

-- 测试集合交集
local intersection = set1 * set2
print("集合交集长度:", #intersection)

-- 测试集合差集
local difference = set1 - set2
print("集合差集长度:", #difference)

-- 测试集合对称差集
local symmetric_diff = set1 ~ set2
print("集合对称差集长度:", #symmetric_diff)

print("\n=== 测试字典操作 ===")

-- 创建字典
local dict1 = {a = 1, b = 2, c = 3}
local dict2 = {b = 20, c = 30, d = 4}

-- 测试字典长度
print("字典1长度:", #dict1)
print("字典2长度:", #dict2)

-- 测试字典访问
print("字典1['a']:", dict1.a)
print("字典2['b']:", dict2.b)

-- 测试字典赋值
dict1.e = 5
print("字典1['e']:", dict1.e)

-- 测试字典合并
local merged = dict1 + dict2
print("合并后字典长度:", #merged)

-- 测试字典交集（键的交集）
local key_intersection = dict1 * dict2
print("键交集长度:", #key_intersection)

-- 测试字典差集（键的差集）
local key_difference = dict1 - dict2
print("键差集长度:", #key_difference)

print("\n=== 测试完成 ===")
