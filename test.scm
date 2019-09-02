(load "stdlib.scm")

"comments"
;; 123
 
 ;

123 ; 123

;; abc

"literals and lists"
-456
'-456
#t
'#t
#f
'#f
"abc"
'"abc"
'()
'(0 . 1)
'(0 1)
'(0 . (1 . ()))
'(0 . (1 . 2))
'(1 2 3)
'(12 -34 #t ab "cd" (1 . 2) (1 2 3))
'x
'ab 'c

"if"
(if #t 1 2)
(if #t 'a 'b)
(if #f 1 2)
(if #t 1)
(if #f 1)
(if 0 1 2)

"primitive procedures"
(+ 1 2)
(+ 3 -1)
(+ 1 2 3)
(+ 1 (+ 2 3))
(- 15 5 5)
(* 1 2 3)
(/ 60 3 2)
(not #t)
(not #f)

"lambda"
(define f (lambda () (lambda (x) x)))
f
(f)
(define g (f))
(g 1)
((f) 2)
((lambda (x) x) 3)

"cond"
(cond (#f 1)
      ((eq? 'a 'a) 2)
      (else 3))

"y combinator"
(define Y
    (lambda (f)
      ((lambda (x) (f (lambda (y) ((x x) y))))
       (lambda (x) (f (lambda (y) ((x x) y)))))))
(define factorial
    (Y (lambda (fact) 
         (lambda (n) 
           (if (= n 0)
               1
               (* n (fact (- n 1))))))))
(factorial 5)

"apply"
(apply + '(1 2 3))
(apply factorial '(5))

"alternate define form"
(define (double x) (* 2 x))
(map double '(1 2 3))

"let"
(let ((x (+ 1 1))
      (y (- 5 2)))
  (+ x y))
  