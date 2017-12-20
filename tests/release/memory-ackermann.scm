

(define (ackermann m n)
  (if (= 0 m) 
	(+ n 1)
	(if (and (> m 0) (= 0 n))
	  (ackermann (- m 1) 1)
	  (ackermann (- m 1) (ackermann m (- n 1))))))

(ackermann 4 4)

