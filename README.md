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
lispy> (+ 1 (* 2 3))  ; => 7
lispy> + 1 (* 2 3)    ; => 7
```

Quoted expressions are denoted by braces:

```lisp
lispy> (+ 1 2 3)      ; => 6
lispy> {+ 1 2 3}      ; => {+ 1 2 3}
lispy> len {+ 1 2 3}  ; => 4
```

Now, let's take it for a spin:

```lisp
lispy> (-> {n} {* n n})               ; => lambda which squares its arg
lispy> (-> {n} {* n n}) -5            ; => 25, calling the lambda directly
lispy> def {square} (-> {n} {* n n})  ; => bind symbol to lambda
lispy> square -5                      ; => 25
```

How about a fancy function defining function?

```lisp
lispy> def {defun} (-> {args body} {def (head args) (-> (tail args) body)})
```

Now:

```lisp
lispy> defun {square x} {* x x}  ; => yum
lispy> square -5                 ; => 25
```

Partial application? Sure:

```lisp
lispy> defun {add n increment} {+ n increment}
lispy> def {inc} (add 1)
lispy> inc 0  ; => 1
```

And some simple currying tricks:

```lisp
lispy> defun {curry f xs} {eval (cons f xs)}
lispy> defun {uncurry f & xs} {f xs}
lispy> curry + {1 2 3}                        ; => 6
lispy> uncurry tail 1 2 3                     ; => {2 3}
```

Hit <kbd>Ctrl</kbd>-<kbd>D</kbd> to exit.

To clean the build:

```
$ make clean
```
