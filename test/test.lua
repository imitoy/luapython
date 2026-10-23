-- Regression tests for the P0 bugs listed in the code review.
-- Run with: make test          (or see test/run.sh for a manual setup)

local pass, fail, skip = 0, 0, 0

local function ok(cond, name, detail)
    if cond then
        pass = pass + 1
        io.write("  ok   ", name, "\n")
    else
        fail = fail + 1
        io.write("  FAIL ", name, detail and ("  <- " .. tostring(detail)) or "", "\n")
    end
end

local function skipped(name)
    skip = skip + 1
    io.write("  skip ", name, "\n")
end

local function section(name)
    io.write("\n[", name, "]\n")
end

-- Version capabilities. Lua 5.1 has both `load` (reader function only) and
-- `loadstring` (string), so loadstring must win there; 5.3+ has only `load`,
-- which does accept a string.
local compile = loadstring or load
local LUA_VER = tonumber((_VERSION:gsub("^Lua ", ""))) or 5.1
-- Bitwise operators `& | ~ << >>` only exist from 5.3 on. Probing with a tiny
-- chunk is more reliable than comparing version strings.
local has_bitops = compile("return 1 & 1") ~= nil

-- Exceptions must not escape into stderr noise during the tests.
local function quiet(fn, ...)
    return fn(...)
end

--------------------------------------------------------------------------
section("加载")

local luapython = require "luapython"
ok(type(luapython) == "table", "require 'luapython'")
local path = luapython.load()
ok(type(path) == "string" and path:match("libpython"), "load() 找到 libpython", path)
ok(luapython.isLoadedNative() == true, "isLoadedNative() 为真")

local math = luapython.import "math"
local builtins = luapython.import "builtins"
local json = luapython.import "json"
local fractions = luapython.import "fractions"

ok(math and math.tan(90) == math.tan(90), "import 'math' 并调用函数")
ok(math.sqrt(16) == 4.0, "math.sqrt(16) == 4.0")

--------------------------------------------------------------------------
section("P0-2 混合算术的操作数顺序")

local half = fractions.Fraction(1, 2)          -- 不会被转成 Lua number

ok(tostring(half - fractions.Fraction(1, 4)) == "1/4", "half - 1/4 == 1/4",
   tostring(half - fractions.Fraction(1, 4)))
ok(tostring(fractions.Fraction(1, 4) - half) == "-1/4", "1/4 - half == -1/4",
   tostring(fractions.Fraction(1, 4) - half))
ok(tostring(half / 2) == "1/4", "half / 2 == 1/4", tostring(half / 2))
ok(tostring(2 / half) == "4", "2 / half == 4", tostring(2 / half))
ok(tostring(half % 1) == "1/2", "half % 1 == 1/2", tostring(half % 1))
ok(tostring(half ^ 2) == "1/4", "half ** 2 == 1/4", tostring(half ^ 2))
ok(tostring(half + half) == "1", "half + half == 1", tostring(half + half))
ok(tostring(-half) == "-1/2", "-half == -1/2", tostring(-half))

-- Lua 5.1 only calls an order metamethod when *both* operands carry the very
-- same `__lt`/`__le`, so `pynum < 5` is rejected by the interpreter before any
-- metamethod runs. Arithmetic metamethods have no such restriction. 5.2+
-- relaxed it to "either operand provides the metamethod".
if LUA_VER >= 5.2 then
    ok(half < 1 and not (half < half), "比较运算顺序正确")
else
    skipped("pynum 与 Lua number 的大小比较（Lua 5.1 的 __lt 语义限制）")
end

-- 位运算符 `& | ~ <<` 是 Lua 5.3 才有的语法。这些断言必须放在字符串里用
-- load() 编译，否则整个文件在 5.1/5.2 上连解析都过不去。
local bitwise = [[
local ok, builtins = ...
local hundred = builtins.int(100)
ok(hundred & 12 == 4, "pynum & lua number（位与）", tostring(hundred & 12))
ok(hundred | 3 == 103, "pynum | lua number（位或）", tostring(hundred | 3))
ok(hundred << 2 == 400, "pynum << 2", tostring(hundred << 2))
ok(~hundred == -101, "~pynum（按位取反）", tostring(~hundred))
]]

if not has_bitops then
    skipped("pynum 的位运算（需要 Lua 5.3+，跳过 4 项）")
else
    local chunk, compile_err = compile(bitwise, "=bitwise")
    if chunk then
        chunk(ok, builtins)
    else
        fail = fail + 1
        io.write("  FAIL 位运算用例无法编译  <- ", tostring(compile_err), "\n")
    end
end

--------------------------------------------------------------------------
section("P0-3 list 下标语义")

local list = builtins.list({ 10, 20, 30 })
ok(list[1] == 10 and list[2] == 20 and list[3] == 30, "l[1..3] 分别取到第 1..3 个元素",
   table.concat({ list[1], list[2], list[3] }, ","))
ok(list[-1] == 30, "l[-1] 取最后一个", tostring(list[-1]))
ok(list[-3] == 10, "l[-3] 取第一个", tostring(list[-3]))
ok(list[0] == nil and list[4] == nil, "越界读返回 nil")
ok(#list == 3, "#l == 3", tostring(#list))

list[1] = "first"
ok(list[1] == "first" and list[2] == 20, "l[1]=x 写到第 1 个元素", tostring(list[1]))
list[-1] = "last"
ok(list[3] == "last", "l[-1]=x 写到最后一个元素", tostring(list[3]))

-- 写 2000 次后值必须仍然正确（引用计数错误的回归）
for i = 1, 2000 do list[1] = ("payload-%05d"):format(i) end
ok(list[1] == "payload-02000", "反复写 list[1] 不损坏数据", tostring(list[1]))

local tuple = luapython.tuple({ "a", "b" })
ok(tuple[1] == "a" and tuple[2] == "b" and #tuple == 2, "tuple 下标与长度")

--------------------------------------------------------------------------
section("P0-1 / P0-6 索引与类型检查")

local fraction = fractions.Fraction(3, 4)
local key_errors = 0
for _, key in ipairs({ "table", "boolean", "nil" }) do
    local value
    if key == "table" then value = {}
    elseif key == "boolean" then value = true
    else value = nil end
    local okc, err = pcall(function() return fraction[value] end)
    if not okc and tostring(err):match("must be a string") then
        key_errors = key_errors + 1
    end
end
ok(key_errors == 3, "非字符串 key 抛出 Lua error（不再段错误）", key_errors)

ok(fraction.numerator == 3, "对象属性读取", tostring(fraction.numerator))
ok(fraction.no_such_member == nil, "缺失属性返回 nil（不再抛异常）")
ok(math.no_such_member == nil, "模块缺失属性返回 nil（import 回退依赖此行为）")
ok(math.pi > 3.14 and math.pi < 3.15, "缺失属性之后照常工作（错误状态未泄漏）")

-- 伪造 metatable：必须被当成普通 userdata
local fake = io.tmpfile()
if fake then
    debug.setmetatable(fake, { __name = "python_object_fake" })
    local okc, err = pcall(luapython.astable, fake)
    ok(not okc and tostring(err):match("not a Python object"), "伪造 __name 不被识别为 Python 对象")
end

--------------------------------------------------------------------------
section("P0-4 Python 异常不再污染后续调用")

local okc, err = pcall(function() return builtins.int("not-a-number") end)
ok(not okc, "Python 异常转成 Lua error")
ok(tostring(err):match("ValueError"), "错误信息包含 Python 异常类型", err)
ok(tostring(err):match("invalid literal"), "错误信息包含 Python 异常内容", err)
ok(math.sqrt(9) == 3.0, "异常之后的下一次调用正常", tostring(math.sqrt(9)))
ok(pcall(function() return math.floor(2.7) end), "异常之后的第三方调用正常")

-- 超大整数：以前会留下 OverflowError 并让后续转换退化
local big = builtins.pow(2, 100)
ok(big ~= nil, "2**100 返回一个对象而不是报错")
ok(math.sqrt(4) == 2.0, "大整数之后 float 仍然正常转换")

--------------------------------------------------------------------------
section("P0-5 基本类型不再泄漏引用")

local function rss_kb()
    local f = io.open("/proc/self/status")
    if not f then return nil end
    local text = f:read("*a")
    f:close()
    return tonumber(text:match("VmRSS:%s*(%d+)"))
end

-- valgrind 会用 redzone 和影子内存抬高 RSS，这个指标在它下面没有意义。
local under_valgrind = (os.getenv("LD_PRELOAD") or ""):lower():find("valgrind", 1, true) ~= nil

local before = rss_kb()
if under_valgrind then
    skipped("30 万次 float 返回不涨 RSS（valgrind 下 RSS 不可信）")
elseif before then
    for i = 1, 300000 do
        local _ = math.sqrt(i)
    end
    collectgarbage("collect")
    local after = rss_kb()
    local grown = after - before
    ok(grown < 2048, "30 万次 float 返回最多增长 2MB（实测 " .. grown .. " KB）", grown .. " KB")
end

--------------------------------------------------------------------------
section("容器 / 调用约定")

local dict = luapython.dict { name = "Alice", age = 18 }
local round_trip = json.loads(json.dumps(dict))
ok(round_trip["name"] == "Alice" and round_trip["age"] == 18, "luapython.dict 转 JSON 再读回",
   json.dumps(dict))
ok(#dict == 2, "#dict == 2", tostring(#dict))

local merged = luapython.dict { a = 1 } + luapython.dict { a = 2, b = 3 }
ok(merged.a == 2 and merged.b == 3, "字典 + 是右值覆盖的合并")

ok(tostring(fractions.Fraction(1, 2) + fractions.Fraction(1, 2)) == "1", "分数相加")

-- 空 table 作为位置参数不能被吞掉
ok(json.dumps({}) == "[]", "空 table 作为位置参数", json.dumps({}))
-- 混合 key 的 table 必须显式报错，不能静默丢数据
local okc2, err2 = pcall(function() return json.dumps({ 1, 2, a = 3 }) end)
ok(not okc2 and tostring(err2):match("luapython.list") ~= nil,
   "混合 key 的 table 不再静默丢数据", err2)

-- set 运算
local setmod = luapython.import "builtins"
local s1, s2 = setmod.set({ 1, 2 }), setmod.set({ 2, 3 })
ok(#(s1 + s2) == 3, "集合 + 是并集", tostring(#(s1 + s2)))
ok(#(s1 * s2) == 1, "集合 * 是交集", tostring(#(s1 * s2)))
ok(#(s1 - s2) == 1, "集合 - 是差集", tostring(#(s1 - s2)))

-- 迭代器
local collected = {}
for value in builtins.iter(builtins.list({ 1, 2, 3 }))() do
    collected[#collected + 1] = value
end
ok(#collected == 3 and collected[1] == 1 and collected[3] == 3, "for ... in pyiter()")

--------------------------------------------------------------------------
section("P0-7 finalize 之后不再崩溃（必须最后执行）")

local survivor = builtins.list({ 1, 2, 3 })
luapython.finalize()
local okc3, err3 = pcall(function() return #survivor end)
ok(not okc3, "finalize 后访问对象抛出 Lua error 而不是崩溃", err3)
collectgarbage("collect")
collectgarbage("collect")
ok(true, "finalize 后 GC 不崩溃")

--------------------------------------------------------------------------
io.write(string.format("\n%d passed, %d failed, %d skipped\n", pass, fail, skip))
if fail > 0 then
    os.exit(1)
end
