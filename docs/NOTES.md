# 问答记录

## Q: 既然 ` 和 quasiquote 等价，为何 system.lsp 还要再定义一次？且定义的很复杂？

Reader 遇到 `` ` `` 只是简单地把语法糖转换成 `(quasiquote ...)` 形式：

```c
// reader 中的处理
case '`':
    getc(f);
    read(f, env);
    push(make_list(QUASIQUOTE, pop(), END));  // `x → (quasiquote x)
```

同理 `,` → `(unquote ...)`、`,@` → `(unquote-splicing ...)`。**reader 只做了一层语法糖替换，并不理解其中的语义**。

如果 reader 做完替换就结束了，求值器遇到 `(quasiquote (1 ,(+ 1 2) 3))` 时，`quasiquote` 只是一个普通的符号——**它必须被定义为一个宏**，在展开阶段把准引用转换成纯粹的列表构造代码。

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

### 为什么复杂

`qq-expand` 需要处理所有边界情况：

```lisp
`x                → (quote x)                           ; 原子直接 quote
`(a ,b c)         → (append (list 'a) (list b) (list 'c))  ; ,b 替换为 b 的值
`(a ,@b c)        → (append (list 'a) b (list 'c))         ; ,@b 拼接 b 的内容
`(a `,b c)        → 嵌套准引用，逐层展开                     ; 嵌套处理
```

这些逻辑包含递归、条件分支和多种组合，用 C 实现会非常繁琐。**用 Lisp 自身来定义 quasiquote 是 Lisp 的传统做法**——语言核心只提供最小原语，复杂功能由自身实现，既灵活又易于维护。
