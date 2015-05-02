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
lispy> + 1 2 3
lispy> (+ 1 2 3)
```

Some of the features so far:

```lisp
lispy> list 1 2 3                                 ; => {1 2 3}
lispy> list -5 {} len                             ; => {-5 {} <len>}
lispy> def {foo bar baz} 1 2 3                    ; => ()
lispy> foo                                        ; => 1
lispy> bar                                        ; => 2
lispy> baz                                        ; => 3
lispy> % baz bar                                  ; => 1
lispy> def {args} {foo bar baz}                   ; => ()
lispy> args                                       ; => {foo bar baz}
lispy> len args                                   ; => 3
lispy> def {args} (cons + args)                   ; => ()
lispy> args                                       ; => {<+> foo bar baz}
lispy> eval args                                  ; => 6
lispy> eval (join args (head (tail {-1 -2 -3})))  ; => 4
lispy> def {foo} -                                ; => ()
lispy> eval (tail args)                           ; => -1
```

Hit <kbd>Ctrl</kbd>-<kbd>D</kbd> to exit.

To clean the build:

```
$ make clean
```
