# Bootstrap Scheme

This is a minimal Scheme interpreter written from scratch in C99. It is able to run the interpreter from section 4.1 of SICP:

## Try it

```
make && ./bss -f sicp-test.scm
```

This will run the following code:

```
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
```

## References
- [SICP 4.1](http://sarabander.github.io/sicp/html/4_002e1.xhtml#g_t4_002e1)
- [Bootstrap Scheme](https://github.com/petermichaux/bootstrap-scheme)