

(define x (make-hash (list (cons 1 2) (cons 3 4))))

(hash-clear! x)

(if (hash-has-key? x 1) 1 2)


