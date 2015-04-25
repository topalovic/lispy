Lispy
=====

Exploring parser combinators through @orangeduck's neat
[C library](https://github.com/orangeduck/mpc) while building a tiny
Lisp along the way. Fun!

You can get your own [here](http://buildyourownlisp.com/contents).


## Usage

Clone the repo and run:

```
$ make run
```

You'll wind up in a simple REPL:

```lisp
lispy> + (* 3 3) (+ 4 4) 5
22
lispy> /
/
lispy>
()
lispy> (% 1 0)
Error: Division by zero
lispy> + ()
Error: Cannot operate on non-number
```

Hit <kbd>Ctrl</kbd>-<kbd>C</kbd> to exit.

To clean the build:

```
$ make clean
```
