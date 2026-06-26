; flisp unit tests
; Usage: ./lisp example.lsp
; Notes:
;   - Use '() for empty list (bare () will error)
;   - Use (- n) for negative numbers (-n is a symbol)
;   - Boolean true is '#t, not 'true

(defun assert (expected actual)
  (if (= expected actual) 'pass
    (do (print 'FAIL) (print '-expected) (print expected)
        (print '-actual) (print actual) 'fail)))

(defun check (label val expected)
  (print label) (assert expected val))

; ============================================================
; 1 Arithmetic
; ============================================================
(print '===Arithmetic===)

(check 'add (+ 1 2) 3)
(check 'add-multi (+ 1 2 3 4) 10)
(check 'add-neg (+ 3 (- 5)) (- 2))
(check 'sub (- 5 3) 2)
(check 'sub-neg (- 5) (- 5))
(check 'sub-multi (- 10 3 2) 5)
(check 'sub-zero (- 0) 0)
(check 'mul (* 2 3) 6)
(check 'mul-multi (* 2 3 4) 24)
(check 'mul-zero (* 0 5) 0)
(check 'div (/ 10 2) 5)
(check 'div-trunc (/ 10 3) 3)
(check 'div-zero (/ 0 5) 0)

; ============================================================
; 2 Comparison
; ============================================================
(print '===Compare===)

(check 'lt (< 2 3) '#t)
(check 'lt-false (< 5 2) nil)
(check 'gt (> 3 2) '#t)
(check 'gt-false (> 1 5) nil)
(check 'eq (= 3 3) '#t)
(check 'eq-false (= 3 4) nil)
(check 'eq-sym (= 'a 'a) '#t)
(check 'eq-sym-false (= 'a 'b) nil)
(check 'eq-list (= '(1 2 3) '(1 2 3)) '#t)
(check 'eq-list-false (= '(1 2) '(1 3)) nil)
(check 'eq-empty (= '() '()) '#t)
(check 'eq-diff-len (= '(1 2) '(1 2 3)) nil)

; ============================================================
; 3 Type Predicates
; ============================================================
(print '===Type===)

(check 'number?-true (number? 42) '#t)
(check 'number?-sym (number? 'x) nil)
(check 'number?-list (number? '(1 2)) nil)
(check 'symbol?-true (symbol? 'hello) '#t)
(check 'symbol?-num (symbol? 42) nil)
(check 'list?-true (list? '(1 2)) '#t)
(check 'list?-empty (list? '()) '#t)
(check 'list?-num (list? 42) nil)
(check 'builtin?-true (builtin? +) '#t)
(check 'builtin?-num (builtin? 42) nil)
(check 'builtin?-sym (builtin? 'x) nil)

; ============================================================
; 4 List Operations
; ============================================================
(print '===List===)

(check 'cons-head (head (cons 1 '(2 3))) 1)
(check 'cons-tail (head (tail (cons 1 '(2 3)))) 2)
(check 'head (head '(a b c)) 'a)
(check 'tail-head (head (tail '(a b c))) 'b)
(check 'tail-single (tail '(a)) '())

; ============================================================
; 5 Logic
; ============================================================
(print '===Logic===)

(check 'not-nil (not nil) '#t)
(check 'not-true (not '#t) nil)
(check 'not-num (not 0) nil)
(check 'and-last (and 1 2 3) 3)
(check 'and-short (and 1 nil 3) nil)
(check 'or-first (or nil 2 3) 2)
(check 'or-all-nil (or nil nil) nil)

; ============================================================
; 6 Quote / Quasiquote
; ============================================================
(print '===Quote===)

(check 'quote-sym 'x 'x)
(check 'quote-list '(1 2 3) '(1 2 3))
(check 'qq-simple `(1 2 3) '(1 2 3))
(def qq-a 10)
(check 'qq-unquote `(x ,qq-a) '(x 10))
(def qq-lst '(1 2 3))
(check 'qq-splice `(0 ,@qq-lst 4) '(0 1 2 3 4))

; ============================================================
; 7 Define / Reassign
; ============================================================
(print '===Define===)

(def d-x 100)
(check 'def d-x 100)
(def d-x (+ d-x 50))
(check 'def-reassign d-x 150)

; ============================================================
; 8 Cond
; ============================================================
(print '===Cond===)

(check 'cond-yes (cond ((< 1 2) 'yes) (else 'no)) 'yes)
(check 'cond-else (cond (nil 'first) (else 'second)) 'second)
(check 'cond-last (cond ((> 1 2) 'first) (else 'last)) 'last)
(check 'cond-truthy (cond ('#t 'branch)) 'branch)

; ============================================================
; 9 Functions
; ============================================================
(print '===Functions===)

(defun add2 (a b) (+ a b))
(check 'defun (add2 3 4) 7)

(defun vari-sum (& xs) (fold + 0 xs))
(check 'rest-args (vari-sum 1 2 3 4 5) 15)

(check 'lambda ((fn (x) (* x 2)) 5) 10)
(check 'nested-fn ((fn (x) ((fn (y) (+ x y)) 3)) 5) 8)

(defun make-adder (x) (fn (y) (+ x y)))
(def add5 (make-adder 5))
(check 'closure (add5 10) 15)

(defun fact (n)
  (if (= n 0) 1 (* n (fact (- n 1)))))
(check 'recursion (fact 6) 720)

(defun sum-to (n acc)
  (if (= n 0) acc (sum-to (- n 1) (+ acc n))))
(check 'tail-rec (sum-to 100 0) 5050)

(defun fib (n)
  (if (< n 2) n (+ (fib (- n 1)) (fib (- n 2)))))
(check 'fib (fib 10) 55)

; ============================================================
; 10 Macros
; ============================================================
(print '===Macros===)

(defmacro twice (expr) (list 'do expr expr))
(twice (print 'twice-ok))

(defmacro my-when (cond & body)
  (list 'cond (list cond (cons 'do body))))
(check 'macro-when (my-when 1 'yes) 'yes)
(check 'macro-when-false (my-when nil 'no) nil)

(defmacro no-eval-macro (x) (list '+ x 1))
(check 'macro-no-eval (no-eval-macro 5) 6)

; ============================================================
; 11 Do
; ============================================================
(print '===Do===)

(check 'do-ret (do 1 2 3) 3)
(check 'do-sym (do 'a 'b 'c) 'c)

; ============================================================
; 12 Eval / Apply
; ============================================================
(print '===Eval===)

(check 'eval (eval '(+ 1 2)) 3)
(check 'eval-list (eval (list '+ 1 2)) 3)
(check 'apply (apply + '(10 20 30)) 60)
(check 'apply-lambda (apply (fn (x y) (+ x y)) '(10 20)) 30)

; ============================================================
; 13 System Library
; ============================================================
(print '===System===)

(check 'list (list 1 2 3) '(1 2 3))
(check 'atom?-true (atom? 42) '#t)
(check 'atom?-list (atom? '(1 2)) nil)
(check 'null?-true (null? '()) '#t)
(check 'null?-false (null? '(1)) nil)

(check 'if-true (if 1 'yes 'no) 'yes)
(check 'if-false (if nil 'yes 'no) 'no)
(check 'if-no-else (if 1 'yes) 'yes)

(check 'fst (fst '(a b c)) 'a)
(check 'snd (snd '(a b c)) 'b)

(check 'map-neg (= (map - '(1 2 3)) (list (- 1) (- 2) (- 3))) '#t)
(check 'map-double (map (fn (x) (* x 2)) '(1 2 3)) '(2 4 6))

(check 'let (let ((x 5) (y 3)) (+ x y)) 8)
(check 'let-single (let ((x 10)) (* x 2)) 20)

(check 'fold-sum (fold + 0 '(1 2 3 4)) 10)
(check 'fold1-sum (fold1 + '(1 2 3 4)) 10)

(check 'any?-true (any? number? '(1 a nil)) '#t)
(check 'any?-false (any? number? '(x y)) nil)

(check 'append-both (append '(1 2) '(3 4)) '(1 2 3 4))
(check 'append-l-empty (append '() '(1 2)) '(1 2))
(check 'append-r-empty (append '(1) '()) '(1))

; ============================================================
; 14 Reader Syntax
; ============================================================
(print '===Reader===)

(print `(a b c))
(print `(a ,(+ 1 2) c))
(print (zip '(1 2 3) '(a b c)))

; ============================================================
; 15 GC Stress
; ============================================================
(print '===GC===)

(defun range (n)
  (if (= n 0) '() (cons n (range (- n 1)))))
(check 'gc (head (range 2000)) 2000)

; ============================================================
; 16 Builtin Display
; ============================================================
(print '===Builtins===)

(print +)
(print cons)
(print head)
(print tail)

(print 'done)
