

(define x (make-vector 3 4))

(define y (vector 1 2 3 4))

(vector-set! x 2 9)

(+ (vector-ref x 2) (vector-ref x 1) (vector-ref y 0))


