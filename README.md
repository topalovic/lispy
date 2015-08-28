Lispy
=====

*“One can make all kinds of Lisps using simple household items.
If one were so inclined.”*

A tiny Lisp(ish) interpreter based on @orangeduck's
[excellent tutorial](http://buildyourownlisp.com/contents), with basic
support for integers, strings, conditionals, user-defined vars and
functions. No macros and TCO yet, but a man can dream...


## Setup

Clone the repo and run:

```sh
$ make run
```

If all is well, you'll wind up in a simple REPL:

```
>>
```

Hit <kbd>Ctrl</kbd>-<kbd>D</kbd> to exit.

To clean the build:

```sh
$ make clean
```


## Usage

With formalities out of the way, let's take it for a spin.

The outermost sexpr allows parens to be omitted, so these are
equivalent:

```lisp
(+ 1 (* 2 3))  ; => 7
+ 1 (* 2 3)    ; => 7
```

Quoted expressions are denoted by braces:

```lisp
(+ 1 2 3)       ; => 6
{+ 1 2 3}       ; => {+ 1 2 3}
eval {+ 1 2 3}  ; => 6
```

To load a source file:

```lisp
load "hello.lsp"
```

To define and call a function:

```lisp
-> {n} {* 2 n}              ; => a lambda that doubles its arg
(-> {n} {* 2 n}) 5          ; => 10, calling it directly
def {dbl} (-> {n} {* 2 n})  ; => bind symbol to lambda
dbl 5                       ; => 10
```

A [utility library](src/prologue.lsp) will be loaded by default,
providing some common functionality. Let's check out some of that
goodness.

Function definition reloaded, now with a friendlier syntax:

```lisp
defn {dbl n} {* 2 n}
dbl 5  ; => 10
```

You can slice and dice...

```lisp
def {array} (range -3 3)
nth 5 array               ; => 2
apply + (but-last array)  ; => -3
split 2 (reverse array)   ; => {{3 2} {1 0 -1 -2 -3}}
```

filter and query...

```lisp
all? num? array          ; => 1 (true)
len (filter odd? array)  ; => 4
drop-while neg? array    ; => {0 1 2 3}
```

map and fold...

```lisp
sum (map sqr array)  ; => 28

zip array (map (-> {n} {abs (* 2 n)}) array)
; => {{-3 6} {-2 4} {-1 2} {0 0} {1 2} {2 4} {3 6}}
```

compose and partially apply functions...

```lisp
(comp dec abs (apply -)) array  ; => 5

defn {add n incr} {+ n incr}
def {++} (add 1)
++ 0  ; => 1
```

and so on.

Type the name of the function to get its definition printed:

```lisp
first  ; => (-> {l} {eval (head l)})
```

and make sure to check the [prologue](src/prologue.lsp) for more
goodies.

Thanks for dropping by! o/
