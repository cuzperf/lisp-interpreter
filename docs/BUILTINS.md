# flisp 内置功能参考手册

flisp 共提供 27 个内置功能（builtin），分为两类：

- **特殊形式 (Special Form)**：参数不自动求值，如 `fn`、`def`、`cond`、`quote` 等
- **普通函数 (Function)**：参数自动求值，如 `+`、`cons`、`head`、`print` 等

特殊形式用来提供语言的核心结构——变量定义、条件分支、函数创建等。普通函数用来操作数据。

> **注意**：布尔值真为 `#t`（符号），假为 `nil`（符号）。空列表 `'()` 也是假值。

---

## 1. 特殊形式

### `quote` — 阻止求值

`(quote expr)` 返回 `expr` 本身，不对其求值。

```lisp
(print (quote a))      ; 输出: a （符号 a，不会去查找 a 变量的值）
(print (quote (1 2)))  ; 输出: (1 2) （列表，不会尝试调用 1 作为函数）
```

**简写语法**：`'expr` 等价于 `(quote expr)`

```lisp
(print 'hello)         ; 输出: hello
(print '(a b c))       ; 输出: (a b c)
(print '())            ; 输出: ()
```

---

### `fn` — 创建函数（λ 表达式）

`(fn (params) body)` 创建一个匿名函数。

```lisp
; 一元函数
(def double (fn (x) (* x 2)))
(print (double 5))                 ; 输出: 10

; 多元函数
(def add (fn (a b) (+ a b)))
(print (add 3 4))                  ; 输出: 7

; 可变参数（使用 & 捕获剩余参数）
(def sum-all (fn (& xs) (fold + 0 xs)))
(print (sum-all 1 2 3 4))          ; 输出: 10

; 匿名函数直接调用
(print ((fn (x) (* x x)) 6))       ; 输出: 36

; 闭包：捕获外部变量
(def make-counter (fn (init)
  (fn () (do (def init (+ init 1)) init))))
(def c (make-counter 0))
(print (c))                        ; 输出: 1
(print (c))                        ; 输出: 2
```

---

### `macro` — 创建宏

`(macro (params) body)` 创建一个宏。宏的参数**不**求值，宏体返回一个 S-表达式，该表达式再被求值。

```lisp
; 定义一个简单宏：将表达式执行两次
(def twice (macro (expr)
  (list 'do expr expr)))

(twice (print 42))                 ; 输出: 42  （打印两次）
                                   ;       42

; 宏的参数是不求值的原始表达式
(def my-if (macro (cond then else)
  (list 'cond (list cond then) (list 'else else))))

(print (my-if (< 1 2) 'yes 'no))  ; 输出: yes
```

---

### `def` — 定义变量

`(def name value)` 定义全局变量。如果变量已存在，则更新其值。

```lisp
(def x 10)                         ; 定义 x = 10
(print x)                          ; 输出: 10

(def x (+ x 1))                    ; 可重新赋值
(print x)                          ; 输出: 11

(def pi 314)                       ; 可定义任意符号
```

---

### `cond` — 条件分支

`(cond (test1 expr1) (test2 expr2) ... (else exprN))` 依次求值各条件，第一个为真的条件对应的表达式被求值并返回。`else` 是兜底分支（其值为 `#t`，永远为真）。

```lisp
(print (cond ((< 1 2) 'yes) (else 'no)))         ; 输出: yes
(print (cond (nil 'case1) ('#t 'case2) (else 'case3)))  ; 输出: case2
(print (cond (nil 'first) (else 'last)))          ; 输出: last
```

> 条件判断规则：只有 `nil` 和 `'()` 为假，其他值（包括 `0`）均为真。

---

### `do` — 顺序执行

`(do expr1 expr2 ... exprN)` 依次求值所有表达式，返回最后一个表达式的值。

```lisp
(print (do 1 2 3))                 ; 输出: 3 （只返回最后一个）
(do (print 'step1) (print 'step2)) ; 输出: step1 （先执行第一个）
                                   ;      step2 （再执行第二个）
```

---

### `and` — 逻辑与（短路求值）

`(and expr1 expr2 ... exprN)` 从左到右求值，遇到假值（`nil` 或 `'()`）就停止并返回该假值；否则返回最后一个表达式的值。

```lisp
(print (and 1 2 3))                ; 输出: 3 （全部为真，返回最后一个）
(print (and 1 nil 3))              ; 输出: nil （遇到 nil 短路）
(print (and 0 1 2))                ; 输出: 2 （0 不是 nil，不会短路）
```

---

### `or` — 逻辑或（短路求值）

`(or expr1 expr2 ... exprN)` 从左到右求值，遇到真值就停止并返回该值；否则返回最后一个（假）值。

```lisp
(print (or nil 2 3))               ; 输出: 2 （遇到 2 短路）
(print (or nil nil))               ; 输出: nil （全假）
(print (or 0 1))                   ; 输出: 0 （0 视为真，短路返回 0）
```

---

## 2. 算术函数

### `+` — 加法

```lisp
(print (+ 1 2))                    ; 输出: 3
(print (+ 1 2 3 4 5))             ; 输出: 15
(print (+ 3 (- 5)))                ; 输出: -2 （负数用 (- n) 表示）
```

### `-` — 减法 / 取负

```lisp
(print (- 10 3))                   ; 输出: 7   （10 - 3 = 7）
(print (- 10 3 2))                 ; 输出: 5   （10 - 3 - 2 = 5）
(print (- 5))                      ; 输出: -5  （单个参数时取负数）
```

### `*` — 乘法

```lisp
(print (* 2 3))                    ; 输出: 6
(print (* 2 3 4))                  ; 输出: 24
(print (* 0 5))                    ; 输出: 0
```

### `/` — 除法（整数除法）

```lisp
(print (/ 10 2))                   ; 输出: 5
(print (/ 10 3))                   ; 输出: 3  （整数除法，向下取整）
; (/ 1 0)                         ; 错误: Division by zero
```

---

## 3. 比较函数

### `<` — 小于

```lisp
(print (< 2 3))                    ; 输出: #t
(print (< 5 2))                    ; 输出: nil
```

### `>` — 大于

```lisp
(print (> 3 2))                    ; 输出: #t
(print (> 1 5))                    ; 输出: nil
```

### `=` — 等于（值相等）

对数字按值比较，对符号按 identity 比较，对列表按元素递归比较。

```lisp
(print (= 3 3))                    ; 输出: #t
(print (= 3 4))                    ; 输出: nil
(print (= 'a 'a))                  ; 输出: #t  （符号相等）
(print (= '(1 2 3) '(1 2 3)))      ; 输出: #t  （列表元素递归相等）
(print (= '(1 2) '(1 3)))          ; 输出: nil
(print (= '() '()))                ; 输出: #t
```

---

## 4. 逻辑函数

### `not` — 逻辑非

```lisp
(print (not nil))                  ; 输出: #t
(print (not '#t))                  ; 输出: nil
(print (not 0))                    ; 输出: nil （0 不是 nil，视为真）
(print (not '()))                  ; 输出: nil... 哦不，实际上 '() 是假的
```

---

## 5. 列表操作函数

### `cons` — 构造列表

`(cons head tail)` 构造一个 `(head . tail)` 的序对。`tail` 必须是一个列表。

```lisp
(print (cons 1 '(2 3)))            ; 输出: (1 2 3)
(print (cons 1 '()))               ; 输出: (1)
```

### `head` — 取列表第一个元素（car）

```lisp
(print (head '(a b c)))            ; 输出: a
(print (head (cons 1 '(2 3))))     ; 输出: 1
; (head '())                       ; 错误: Trying to take head/tail of empty list
```

### `tail` — 取列表剩余部分（cdr）

```lisp
(print (tail '(a b c)))            ; 输出: (b c)
(print (tail '(a)))                ; 输出: ()
; (tail '())                       ; 错误: Trying to take head/tail of empty list
```

---

## 6. 类型谓词

### `number?` — 是否为数字

```lisp
(print (number? 42))               ; 输出: #t
(print (number? 'x))               ; 输出: nil
(print (number? '(1 2)))           ; 输出: nil
```

### `symbol?` — 是否为符号

```lisp
(print (symbol? 'hello))           ; 输出: #t
(print (symbol? 42))               ; 输出: nil
(print (symbol? '#t))              ; 输出: #t
```

### `list?` — 是否为列表

```lisp
(print (list? '(1 2)))             ; 输出: #t
(print (list? 42))                 ; 输出: nil
(print (list? '()))                ; 输出: #t （空列表也是列表）
```

### `builtin?` — 是否为内置函数

```lisp
(print (builtin? +))               ; 输出: #t
(print (builtin? 42))              ; 输出: nil
(print (builtin? cons))            ; 输出: #t
```

---

## 7. 元编程函数

### `eval` — 显式求值

`(eval expr)` 对表达式进行求值。

```lisp
(print (eval '(+ 1 2)))            ; 输出: 3
(print (eval (list '+ 1 2)))       ; 输出: 3 （动态构造表达式再求值）

; 与 def 结合实现动态变量访问
(def myvar 42)
(print (eval 'myvar))              ; 输出: 42
```

### `apply` — 以列表形式传递参数

`(apply fn arglist)` 以列表中的元素作为参数调用函数。

```lisp
(print (apply + '(10 20 30)))      ; 输出: 60
(print (apply * '(2 3 4)))         ; 输出: 24
(print (apply (fn (x y) (+ x y)) '(10 20)))  ; 输出: 30
```

---

## 8. `print` — 输出

`(print value1 value2 ...)` 打印值到标准输出，每个值占一行，返回 `nil`。

```lisp
(print 42)                         ; 输出: 42
(print '(a b c))                   ; 输出: (a b c)
(print +)                          ; 输出: #<builtin + >
(print 'hello)                     ; 输出: hello
(print '())                        ; 输出: ()
```

---

## 内置函数速查表

| 名称 | 类型 | 参数 | 说明 |
|------|------|------|------|
| `quote` | 特殊 | 1 | 返回参数本身，不求值 |
| `fn` | 特殊 | 变参 | 创建函数 |
| `macro` | 特殊 | 变参 | 创建宏 |
| `def` | 特殊 | 2 | 定义/更新全局变量 |
| `cond` | 特殊 | 变参 | 条件分支 |
| `do` | 特殊 | 变参 | 顺序执行，返回最后一个值 |
| `and` | 特殊 | 变参 | 短路与 |
| `or` | 特殊 | 变参 | 短路或 |
| `+` | 函数 | ≥1 | 加法 |
| `-` | 函数 | ≥1 | 减法/取负 |
| `*` | 函数 | ≥1 | 乘法 |
| `/` | 函数 | ≥1 | 整数除法 |
| `<` | 函数 | 2 | 小于比较 |
| `>` | 函数 | 2 | 大于比较 |
| `=` | 函数 | 2 | 相等比较 |
| `not` | 函数 | 1 | 逻辑非 |
| `cons` | 函数 | 2 | 构造列表 |
| `head` | 函数 | 1 | 取首元素 |
| `tail` | 函数 | 1 | 取剩余列表 |
| `eval` | 函数 | 1 | 显式求值 |
| `apply` | 函数 | 2 | 以列表参数调用 |
| `list?` | 函数 | 1 | 列表判断 |
| `symbol?` | 函数 | 1 | 符号判断 |
| `number?` | 函数 | 1 | 数字判断 |
| `builtin?` | 函数 | 1 | 内置函数判断 |
| `print` | 函数 | 变参 | 打印输出 |

---

## 系统库常用函数

除了 27 个内置功能，`system.lsp` 还定义了一些常用函数和宏：

| 名称 | 说明 |
|------|------|
| `defun` | 宏，定义函数：`(defun name args body)` |
| `defmacro` | 宏，定义宏：`(defmacro name args body)` |
| `if` | 宏，条件判断：`(if cond then else)` |
| `list` | 函数，创建列表：`(list a b c)` |
| `atom?` | 函数，判断是否为原子（非列表） |
| `null?` | 函数，判断是否为空列表 |
| `map` | 函数，映射：`(map f '(1 2 3))` |
| `let` | 宏，局部绑定：`(let ((x 1)) body)` |
| `fold` | 函数，左折叠 |
| `fold1` | 函数，以首元素为初值的左折叠 |
| `append` | 函数，拼接两个列表 |
| `any?` | 函数，任一元素满足谓词 |
| `zip` | 函数，多个列表拉链合并 |
| `quasiquote` | 宏，准引用（`` ` `` 语法的底层实现） |
