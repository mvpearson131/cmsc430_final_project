

(define x (make-hash (list (cons 1 2) (cons 3 4))))

(hash-set! x 2 5)

(+ (hash-ref x 1) (hash-ref x 2) (hash-ref x 3))


