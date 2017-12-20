

(define x (make-hash (list (cons 1 2))))

(define y (vector 1 2))

(if (and (hash? x) 
		 (not (hash? y))
		 (hash-has-key? x 1)
		 (not (hash-has-key? x 2))) 1 2)


