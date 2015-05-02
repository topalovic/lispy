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
lispy> list 1 2 3 4
=> {1 2 3 4}
lispy> join {+ 1 2} (head {3 4 5})
=> {+ 1 2 3}
lispy> eval (join {+ 1 2} (head {3 4 5}))
=> 6
lispy> tail {tail tail tail}
=> {tail tail}
lispy> cons (len 0 0 0) {2 1}
=> {3 2 1}
```

Hit <kbd>Ctrl</kbd>-<kbd>C</kbd> to exit.

To clean the build:

```
$ make clean
```
