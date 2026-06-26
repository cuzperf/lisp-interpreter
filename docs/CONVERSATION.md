# flisp 问题研讨全记录

## 目录

1. [单元测试与代码审查](#1-单元测试与代码审查)
2. [Builtin 内置功能详解](#2-builtin-内置功能详解)
3. [Quasiquote 的实现原理](#3-quasiquote-的实现原理)
4. [GC 堆变量含义与 realloc 问题](#4-gc-堆变量含义与-realloc-问题)
5. [GC 性能优化讨论](#5-gc-性能优化讨论)
6. [附录：已生成的文件](#6-附录已生成的文件)

---

## 1. 单元测试与代码审查

### 任务

为 flisp 编写单元测试保存到 `example.lsp`，提高代码覆盖度。不修改任何代码，如发现问题单独生成报告。

### 分析过程

审查了所有源文件（lisp.h, lisp_core.c, lisp_eval.c, lisp_read.c, lisp_print.c, lisp_list.c, lisp_symbol.c, lisp_dump.c, lisp_repl.c, lisp_main.c）和 system.lsp。

### 发现的 Bug

写入 `BUGS.md`（5 个问题）：

1. **GC env 括号错误** (`lisp_core.c:180`)
   ```c
   // 错误：括号位置导致 type_of 的参数是布尔值
   if (type_of(g_env_stack[ee] != TYPE_SYM)) {
   // 应为：
   if (type_of(g_env_stack[ee]) != TYPE_SYM) {
   ```

2. **read_int 不校验数字字符** (`lisp_read.c:68-71`) — 遇 `.` 等字符静默错误解析

3. **不支持负数直接量** — `-5` 被读为符号而非数字

4. **UNBOUND 死代码** (`lisp_eval.c:467`)

5. **read_int EOF 死循环** — 文件末尾数字无换行时可能卡死

### 测试要点

- 不使用 `()` 裸空列表作表达式（会触发 `head/tail of empty list` 错误）
- 不使用 `-5` 字面量（被读为符号），用 `(- 5)` 代替
- 布尔真值是 `#t`，不是 `true`
- `list?` 结果：`'()` 是列表，`list?` 返回 `#t`

### 测试结果

全部通过，无 FAIL。

---

## 2. Builtin 内置功能详解

### 内置功能分类

flisp 共 27 个内置功能：

**特殊形式（8个）**：参数不自动求值
- `quote` / `'` — 阻止求值
- `fn` — 创建函数（λ 表达式）
- `macro` — 创建宏
- `def` — 定义/更新全局变量
- `cond` — 条件分支
- `do` — 顺序执行，返回最后一个值
- `and` — 短路与
- `or` — 短路或

**普通函数（19个）**：参数自动求值
- 算术：`+` `-` `*` `/`
- 比较：`<` `>` `=`
- 逻辑：`not`
- 列表：`cons` `head` `tail`
- 元编程：`eval` `apply`
- 类型谓词：`number?` `symbol?` `list?` `builtin?`
- 输出：`print`

### 负数表示

不支持 `-5` 字面量，必须用 `(- 5)` 表达式。

### 布尔值

- 真：`#t`（符号）
- 假：`nil`（符号）或 `'()`（空列表）
- 判断规则：只有 `nil` 和 `'()` 为假，其他值（包括 0）均为真

详细文档已写入 `BUILTINS.md`。

---

## 3. Quasiquote 的实现原理

### Q: `` ` `` 和 `quasiquote` 等价，为何 system.lsp 还要再定义一次？且定义的很复杂？

Reader 遇到 `` ` `` 只是简单地将语法糖转换成 `(quasiquote ...)` 形式：

```c
case '`':
    getc(f);
    read(f, env);
    push(make_list(QUASIQUOTE, pop(), END));  // `x → (quasiquote x)
```

同理 `,` → `(unquote ...)`、`,@` → `(unquote-splicing ...)`。

**reader 只做了一层语法糖替换，并不理解其中的语义。** 求值器遇到 `(quasiquote ...)` 时，`quasiquote` 只是一个普通符号——**它必须被定义为一个宏**，在展开阶段把准引用转换成纯粹的列表构造代码。

### 展开流程

```
`(1 ,(+ 1 2) 3)
  ↓ reader 转换
(quasiquote (1 ,(+ 1 2) 3))
  ↓ quasiquote 宏展开
(qq-expand (1 ,(+ 1 2) 3))
  ↓ qq-expand 递归处理
(append (list 1) (list (+ 1 2)) (list 3))
  ↓ 正常求值
(1 3 3)
```

### 为何复杂

`qq-expand` 需要处理所有边界情况：

```lisp
`x                → (quote x)                            ; 原子直接 quote
`(a ,b c)         → (append (list 'a) (list b) (list 'c))   ; ,b 替换为 b 的值
`(a ,@b c)        → (append (list 'a) b (list 'c))          ; ,@b 拼接 b 的内容
`(a `,b c)        → 嵌套准引用，逐层展开                      ; 嵌套处理
```

用 C 实现这些逻辑会非常繁琐。**用 Lisp 自身来定义 quasiquote 是 Lisp 的传统做法** — 语言核心只提供最小原语，复杂功能由自身实现。

---

## 4. GC 堆变量含义与 realloc 问题

### Q: 各内存变量的含义？

```c
memory_t g_heap;        // 当前"活动堆"（from-space），分配和 GC 都基于它
memory_t g_curheap;     // 当前堆中的空闲位置指针，halloc 从这里分配
static memory_t g_newheap;   // GC 时的"目标堆"（to-space），存活对象搬到这里
static memory_t g_lim;       // g_heap + g_heap_size，堆的末尾边界
static memory_t g_gc_tresh;  // 声明了但从未使用
```

GC 采用 **stop-and-copy** 策略：每次 GC 把存活对象从 `g_heap`（from-space）搬到 `g_newheap`（to-space），然后交换二者角色。

### Q: realloc 可能返回相同指针，此时完全无需做后续的更新操作？

关键代码：

```c
g_heap_size = (int)(g_heap_size * HEAP_RESIZE_RATIO);
if (g_newheap == NULL) {
    g_newheap = malloc(g_heap_size);
} else {
    g_newheap = realloc(g_newheap, g_heap_size);
}

memory_t t = g_heap;
g_heap = g_newheap;   // 交换
g_newheap = t;

g_curheap = g_heap;   // 重置分配指针
```

**即使 realloc 返回相同指针，也必须交换并重新搬迁。** 原因：

1. `g_newheap` 里保留的是**上一轮 GC 搬过来的数据**，不代表本轮存活数据。
2. 本轮存活对象在 `g_heap`（旧 from-space）里。搬迁循环会把所有可达对象从旧 `g_heap` 拷贝到新 `g_heap`（即 realloc 返回的地址）的开头。
3. `g_curheap` 已重置到新 to-space 起点，**从头覆写旧数据**。

即使堆不需要扩容（原地 realloc），GC 仍然要执行——因为程序可能产生了垃圾，需要回收自由空间。**stop-and-copy 通过压缩堆来实现回收，这和 realloc 是否移动指针无关。**

---

## 5. GC 性能优化讨论

### Q: 这样做会有性能问题吗？

### 当前实现的性能问题

**1. 每次 GC 都复制所有存活对象**

长生命周期对象（如 `system.lsp` 中定义的函数体）在每次 GC 中都被复制一次，产生大量无意义复制。

**2. 堆每次都翻倍**

```c
HEAP_RESIZE_RATIO = 2.0
```

初始 64KB → 128KB → 256KB → 512KB……做几次 GC 后堆就膨胀到 MB 级，大部分空间未被使用。

**3. 根扫描范围过大**

`relocate_symtab` 遍历整个符号表 BST，但绝大多数符号的 binding 是非列表值（数字、符号等），`relocate` 对其无操作——这些遍历纯属浪费。

### 改进方案对比

| 方案 | 实现量 | 长生命周期开销 | 暂停时间 | 空间效率 |
|------|--------|---------------|---------|---------|
| 当前 stop-and-copy | — | 每次复制，高 | 长 | 差（翻倍） |
| **标记-清除** | ~50 行 C | **零** | 中等 | 好 |
| 分代 GC | ~300 行 C | **零（一次复制）** | 短 | 好 |
| 增量式 | ~500 行 C | 低 | 极短（≤1ms） | 好 |

**标记-清除（Mark-Sweep）** 是最务实的选择：
- 不复制对象，只在原地标记存活然后回收未标记的
- 长生命周期对象零开销
- 无需堆翻倍
- List 都是固定大小（16 字节），碎片不是问题

**简单做法**：修复括号 bug + 将翻倍率从 2.0 降至 1.5，减少过度分配。

---

## 6. 附录：已生成的文件

| 文件 | 说明 |
|------|------|
| `example.lsp` | 单元测试文件（49 项测试，全部通过） |
| `BUGS.md` | 代码审查报告（5 个 Bug） |
| `BUILTINS.md` | 内置功能参考手册（27 个 builtin） |
| `NOTES.md` | Quasiquote 问答记录 |
| `CONVERSATION.md` | 本文件（完整对话记录） |
