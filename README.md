Lispy
=====

Exploring parser combinators through @orangeduck's slick
[library](https://github.com/orangeduck/mpc)
while building a tiny Lisp along the way. Fun!

You can get your own [here](http://buildyourownlisp.com/contents).


## Usage

Clone the repo and run:

```
$ make run
```

You'll wind up in a simple REPL:

```lisp
lispy>
```

The outermost sexpr allows parens to be omitted, so these are
equivalent:

```lisp
(+ 1 (* 2 3))  ; => 7
+ 1 (* 2 3)    ; => 7
```

Quoted expressions are denoted by braces:

```lisp
(+ 1 2 3)      ; => 6
{+ 1 2 3}      ; => {+ 1 2 3}
len {+ 1 2 3}  ; => 4
```

Now, let's take it for a spin:

```lisp
(-> {n} {* n n})               ; => lambda which squares its arg
(-> {n} {* n n}) -5            ; => 25, calling the lambda directly
def {square} (-> {n} {* n n})  ; => bind symbol to lambda
square -5                      ; => 25
```

How about a fancy function defining function?

```lisp
def {defun} (-> {args body} {def (head args) (-> (tail args) body)})
```

Now:

```lisp
defun {square x} {* x x}  ; => yum
square -5                 ; => 25
```

Partial application? Sure:

```lisp
defun {add n increment} {+ n increment}
def {inc} (add 1)
inc 0  ; => 1
```

Simple currying tricks:

```lisp
defun {curry f xs} {eval (cons f xs)}
defun {uncurry f & xs} {f xs}
curry + {1 2 3}                        ; => 6
uncurry tail 1 2 3                     ; => {2 3}
```

First take on `map`:

```lisp
(defun {empty? c}
  {== 0 (len c)})

(defun {map f c} {
  if (empty? c)
    {{}}
    {join (list (f (eval (head c)))) (map f (tail c))}})

(map square {-1 2 4})  ; => {1 4 16}
```

and `reduce`:

```lisp
(defun {reduce f acc c} {
  if (empty? c)
    {acc}
    {reduce f (f acc (eval (head c))) (tail c)}})

(defun {sum c}
  {reduce + 0 c})

(sum {-1 2 4})  ; => 5
```

Hit <kbd>Ctrl</kbd>-<kbd>D</kbd> to exit.

To clean the build:

```
$ make clean
```
