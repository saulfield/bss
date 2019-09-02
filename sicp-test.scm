;; Runs the Scheme code for the SICP evaluator, which defines the procedure eval-expr,
;; then evaluates the Y combinator form of the factorial function using the SICP evaluator.

(load "sicp.scm")

(eval-expr '(define Y
              (lambda (F)
                ((lambda (x)
                  (F (lambda (arg) ((x x) arg))))
                (lambda (x)
                  (F (lambda (arg) ((x x) arg)))))))
           global-env)

(eval-expr '(define fact-gen
              (lambda (f)
                (lambda (n)
                  (if (= n 0)
                      1
                      (* n (f (- n 1)))))))
           global-env)

(eval-expr '(define fact (Y fact-gen)) global-env)
(eval-expr '(fact 5) global-env)