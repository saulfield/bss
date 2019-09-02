(define map
  (lambda (f xs)
    (if (null? xs)
        '()
        (cons (f (car xs))
              (map f (cdr xs))))))

(define not
  (lambda (x)
    (if x
        #f
        #t)))