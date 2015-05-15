;;; prologue

;; atoms

(def {nil} {})
(def {true} 1)
(def {false} 0)

;; aliases

(def {eq} ==)
(def {fn} ->)

;; defn
(def {defn} (fn {sig body} {
  def (head sig) (fn (tail sig) body)
}))

;; let
(defn {let x} {
  ((fn {_} x) ())
})

;; when
(defn {when cnd then} {
  if cnd then nil
})

;; unless
(defn {unless cnd then else} {
  if cnd else then
})

;; basic predicates

(defn {nil? x} { eq x nil })
(defn {true? x} { > x false })
(defn {false? x} { eq x false })

(defn {zero? x} { eq 0 x })
(defn {pos? x} { > x 0 })
(defn {neg? x} { < x 0 })

(defn {odd? x} { % (abs x) 2 })
(defn {even? x} { not (odd? x) })

(defn {num? x} {
  eq "number" (type x)
})

(defn {str? x} {
  eq "string" (type x)
})

(defn {list? x} {
  in? {"sexpr" "qexpr"} (type x)
})

;; logical ops

(def {or} +)
(def {and} *)
(defn {not x} {- 1 x})

;; num utils

(defn {inc n} {+ n 1})
(defn {dec n} {- n 1})
(defn {sqr n} {* n n})

(defn {abs n} {
  if (pos? n) {n} {- n}
})

;; list utils

(def {empty?} nil?)

(defn {count l} {
  if (empty? l)
    {0}
    {inc (count (tail l))}
})

(defn {first l}  { eval (head l)} )
(defn {second l} { eval (head (tail l)) })

(defn {nth n l} {
  if (zero? n)
    {first l}
    {nth (dec n) (tail l)}
})

(defn {last l} { nth (dec (len l)) l })

(defn {but-last l} {
  if (empty? (tail l))
    {nil}
    {join (head l) (but-last (tail l))}
})

(defn {take n l} {
  if (and n (len l))
    {join (head l) (take (dec n) (tail l))}
    {nil}
})

(defn {drop n l} {
  if (and n (len l))
    {drop (dec n) (tail l)}
    {l}
})

(defn {split n l} {
  list (take n l) (drop n l)
})

(defn {in? l x} {
  if (empty? l)
    {false}
    {if (eq (first l) x)
      {true}
      {in? (tail l) x}}
})

(defn {reverse l} {
  if (empty? l)
    {nil}
    {join (reverse (tail l)) (head l)}
})

(defn {flatten l} {
  if (empty? l)
    {nil}
    {if (list? l)
      {join (flatten (first l)) (flatten (tail l))}
      {list l}}
})

(defn {range start end} {
  if (> start end)
    {nil}
    {join (list start) (range (inc start) end)}
})

;; functional

(defn {apply f xs} {
  eval (cons f xs)
})

(defn {pack f & xs} {f xs})
(defn {flip f x y} {f y x})

(defn {do & l} {
  if (empty? l)
    {nil}
    {last l}
})

(defn {map f l} {
  if (empty? l)
    {nil}
    {join (list (f (first l))) (map f (tail l))}
})

(defn {foldl f z l} {
  if (empty? l)
    {z}
    {foldl f (f z (first l)) (tail l)}
})

(defn {foldr f z l} {
  if (empty? l)
    {z}
    {f (first l) (foldr f z (tail l))}
})

(def {reduce} foldl)

(defn {sum l} { reduce + 0 l })
(defn {product l} { reduce * 1 l })

; recursive variant, sums trees
(defn {sum2 l} {
  if (num? l)
    {l}
    {apply + (map sum2 l)}
})

(defn {filter f l} {
  if (empty? l)
    {nil}
    {join (if (f (first l)) {head l} {nil}) (filter f (tail l))}
})

(defn {all? f l} {
  apply and (map f l)
})

(defn {any? f l} {
  apply or (map f l)
})

(defn {take-while f l} {
  unless (apply f (head l))
    {nil}
    {join (head l) (take-while f (tail l))}
})

(defn {drop-while f l} {
  unless (apply f (head l))
    {l}
    {drop-while f (tail l)}
})

(defn {zip x y} {
  if (any? empty? {x y})
    {nil}
    {join (list (join (head x) (head y))) (zip (tail x) (tail y))}
})

(defn {comp & fs} {
  (fn {fs args} {
    foldr (fn {z g} {z g}) ((last fs) args) (but-last fs)
  }) fs
})
