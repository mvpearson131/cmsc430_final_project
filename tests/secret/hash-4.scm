

(define x (make-hash (list (cons 1 2))))

(define y (hash-ref x 1))

(hash-remove! x 1)

(if (hash-has-key? x 1) 1 y)


