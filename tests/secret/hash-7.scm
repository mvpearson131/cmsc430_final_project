

(define x (make-hash (list (cons 1 2) (cons 2 3) (cons 5 6))))
(define y (make-hash))

(if (and (= 3 (hash-count x)) (hash-empty? y) (not (hash-empty? x))) 1 2)


