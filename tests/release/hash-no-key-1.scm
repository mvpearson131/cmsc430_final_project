

(let* ([x (make-hash (list (cons 1 2)))] [y (hash-ref x 1)]) 
  (hash-set! x 1 5)
  (define z (hash-ref x 1))
  (+ y z (hash-ref x 7)))


