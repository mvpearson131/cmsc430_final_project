

(define x (make-hash (list (cons 1 0) (cons 2 0) (cons 3 0))))
(define y (make-hash (list (cons 2 0) (cons 1 0))))

(if (and (hash-keys-subset? y x) (not (hash-keys-subset? x y))) 1 2)


