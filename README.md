Lispy
=====

Exploring parser combinators through @orangeduck's neat
[C library](https://github.com/orangeduck/mpc) and building a tiny
Lisp along the way. Fun!

Get your own [here](http://buildyourownlisp.com/contents).


## Usage

Clone the repo and run:

```
$ make run
```

You'll wind up in a simple REPL:

```lisp
lispy> + 1 (* 2 3)
7
lispy> / 1 0
Error: Division by zero!
```

Hit <kbd>Ctrl</kbd>-<kbd>C</kbd> to exit.

To clean the build:

```
$ make clean
```
