# Lisp 解释器 — 内置函数与核心概念

## do — 顺序求值，返回最后一个

**语法：** `(do expr1 expr2 ... exprn)`

对每个子表达式依次求值，返回最后一个表达式的值。

**源码** (`src/lisp_eval.c:323-328`)：
```c
case B_DO:
    push_reverse_list(args);      // 参数逆序压栈
    while (g_sp > ss) {           // 依次求值每个表达式
        res = eval(pop());
    }
    break;
```

**示例：**
```lisp
(do 1 2 3)                          ; => 3
(do (print 'a) (print 'b) (+ 1 2))  ; 输出 a b，返回 3
```

---

## eval — 显式求值一个表达式

**语法：** `(eval expr)`

将运行时构造的数据当做代码执行。宏展开结果的自动求值是解释器内部行为，`eval` 是暴露给用户的接口。

**源码** (`src/lisp_eval.c:284-287`)：
```c
case F_EVAL:
    assert_nargs(1);
    res = eval(g_stack[ss]);  // 对参数直接调用 eval
    break;
```

**示例：**
```lisp
(eval '(+ 1 2))          ; => 3
(eval (list '+ 1 2))     ; => 3（动态构造列表再求值）

(def expr '(+ 1 2))
(eval expr)              ; => 3

; 根据运行时数据动态构造代码并执行
(def op '+)
(def args '(10 20))
(eval (cons op (list (eval (head args)) (eval (head (tail args))))))  ; 30
```

---

## apply — 以列表形式传递参数调用函数

**语法：** `(apply fn arglist)`

把 `arglist` 中的元素展开作为参数传给 `fn`。

**源码** (`src/lisp_eval.c:288-293`)：
```c
case F_APPLY:
    fun = pop();          // 取出函数
    is_apply = true;      // 标记：跳过对实参的再次求值
    goto apply_top;       // 跳转到 apply_top 复用普通调用流程
```

**示例：**
```lisp
(apply + '(10 20 30))                   ; => 60
(apply (fn (x y) (+ x y)) '(10 20))    ; => 30
```

### 为什么需要 apply？

普通调用 `(f a b c)` 参数在源码中逐个写出；`apply` 处理参数已在列表变量中的情况。

**场景：运行时动态生成的参数列表**
```lisp
(def args '(10 20 30))   ; 运行时得到，不知道长度
(apply + args)           ; 60 — 无论列表多长
```

**场景：zip 函数（system.lsp:80）**
```lisp
(defun zip l
  (if (any? null? l)
      '()
      (cons (map head l)
            (apply zip (map tail l)))))  ; 必须用 apply
```
`(map tail l)` 返回 `((b c) (y z))`，`apply zip` 将其展开为 `(zip '(b c) '(y z))`。

---

## 递归

### 普通递归（非尾递归）

递归返回后还有操作，每层保留栈帧，空间 O(n)。

```lisp
(defun fact (n)
  (if (= n 0) 1 (* n (fact (- n 1)))))

; fact(5) 执行过程：
; fact(5) -> (* 5 (fact 4)) -> (* 5 (* 4 (fact 3))) -> ... -> (* 5 (* 4 (* 3 (* 2 1))))
```

### 尾递归（等价于循环）

最后一步直接调用自身，解释器有**尾调用优化（TCO）**，复用栈帧，空间 O(1)。

```c
// lisp_eval.c:43
#define tail_eval(exp) do { sexp = (exp); restore_stack(ss); goto eval_top; } while(0);
```

```lisp
(defun fact-tail (n acc)
  (if (= n 0) acc (fact-tail (- n 1) (* n acc))))
; => 编译等价于 while(n>0) { acc*=n; n--; } return acc;
```

### 递归模拟循环

```lisp
(defun for-range (i n fn)
  (if (< i n)
      (do (fn i)
          (for-range (+ i 1) n fn))
      'done))
```

---

## 循环

解释器没有 `for`/`while` 关键字，循环通过以下方式实现：

1. **尾递归** — 最高效，等价于底层循环
2. **高阶函数** `map`/`fold` — 函数式遍历
3. **用宏定义 while** — 展开为递归调用

```lisp
; map
(map (fn (x) (* x 2)) '(1 2 3 4))   ; => (2 4 6 8)

; fold
(fold + 0 '(1 2 3 4 5))             ; => 15
```
