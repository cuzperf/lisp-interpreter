# flisp 代码审查报告

## Bug 1: GC 中 env_stack 类型检查括号错误

**文件**: `src/lisp_core.c:180`

**代码**:
```c
if (type_of(g_env_stack[ee] != TYPE_SYM)) {
    g_env_stack[ee] = relocate(g_env_stack[ee]);
}
```

**问题**: 括号位置错误，`!=` 比较优先级高于 `type_of()` 函数调用。实际执行的是 `type_of(0)` 或 `type_of(1)`，而非 `type_of(g_env_stack[ee])`。

**应改为**:
```c
if (type_of(g_env_stack[ee]) != TYPE_SYM) {
    g_env_stack[ee] = relocate(g_env_stack[ee]);
}
```

**影响**: 由于 `type_of(0) = TYPE_NUM ≠ TYPE_SYM` 且 `type_of(1) = TYPE_LIST ≠ TYPE_SYM`，条件始终为真，所有 env 条目都会被 `relocate`。但 `relocate` 对非 LIST 类型返回原值，因此该 bug 在大多数情况下不影响正确性。

---

## Bug 2: `read_int` 不校验数字字符

**文件**: `src/lisp_read.c:63-74`

**代码**:
```c
static void read_int(FILE* f, Symbol** env)
{
    UNUSED(env);
    int n = 0;
    char c = (char)fgetc(f);
    while (!is_space(c) && c != ')' && c != '(') {
        n = 10 * n + (c - 0x30);
        c = (char)fgetc(f);
    }
    ungetc(c, f);
    push(number(n));
}
```

**问题**: `while` 循环没有检查字符是否为数字（'0'-'9'），遇到非数字字符（如 `.`、`-` 等）时会错误地解析。例如输入 `1.2` 时，`.` 的 ASCII 码为 46，计算 `10 * n + (46 - 48) = 10*1 - 2 = 8`，导致 `1.2` 被读为整数 8。

**影响**: 浮点数或包含非数字字符的输入会导致静默错误解析。

**建议**:
```c
while (c >= '0' && c <= '9') {
    n = 10 * n + (c - '0');
    c = (char)fgetc(f);
}
```

---

## Bug 3: 不支持负数直接量

**文件**: `src/lisp_read.c:149-152`

**代码**:
```c
case '0': case '1': case '2': case '3': case '4':
case '5': case '6': case '7': case '8': case '9':
    read_int(f, env);
    break;
```

**问题**: 只处理以数字（0-9）开头的字面量，不支持负数直接量如 `-5`。`-5` 会被读为符号 `-5`（由 `read_sym` 处理），而非负数。

**影响**: 用户必须使用 `(- 5)` 表达式来表示负数，无法使用 `-5` 字面量。

**建议**: 在 `case '-'` 分支中检查后续字符是否为数字，若是则读取负数。

---

## Bug 4: `eval_sym` 中 UNBOUND 检查为死代码

**文件**: `src/lisp_eval.c:467-468`

**代码**:
```c
if (v == UNBOUND) {
    error("Cannot eval unbound");
}
```

**问题**: `UNBOUND` 定义为 `(value_t)TAG_SYM` = `(value_t)2`。所有合法的 symbol 值都是指针 + 标签（pointer | 0x2），指针至少 4 字节对齐，因此不会恰好等于 2。该条件永远不可能为真。

**影响**: 无害的死代码，但会给代码维护带来困惑。

**建议**: 删除或修改为检查符号的 binding 值。

---

## Bug 5: read_int 到 EOF 时可能死循环

**文件**: `src/lisp_read.c:68-71`

**问题**: 当读取数字时遇到 EOF，`fgetc` 返回 `-1` (EOF)。`(char)-1` 不是空格、`)` 或 `(`，因此循环不会终止。后续 `fgetc` 调用将持续返回 EOF，导致死循环。

**影响**: 在文件末尾书写数字表达式（无换行）时可能导致程序卡死。

**建议**: 在 `while` 循环中添加 `!feof(f)` 或 `c != EOF` 检查。

---

## 总结

| Bug | 文件 | 行 | 严重程度 |
|-----|------|-----|---------|
| GC env 括号错误 | lisp_core.c | 180 | 中 |
| read_int 非数字字符 | lisp_read.c | 68-71 | 中 |
| 不支持负数直接量 | lisp_read.c | 149-152 | 低 |
| UNBOUND 死代码 | lisp_eval.c | 467-468 | 低 |
| read_int EOF 死循环 | lisp_read.c | 68-71 | 中 |
