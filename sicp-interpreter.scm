;; Utility -------------------------------------------------

(define (true? x) (not (eq? x #f)))
(define (false? x) (eq? x #f))

(define (tagged-list? exp tag)
  (if (pair? exp)
      (eq? (car exp) tag)
      #f))

(define (if? exp) (tagged-list? exp 'if))
(define (if-predicate   exp) (cadr  exp))
(define (if-consequent  exp) (caddr exp))
(define (if-alternative exp)
  (if (not (null? (cdddr exp)))
      (cadddr exp)
      'false))

(define (make-procedure params body env)
  (list 'procedure params (scan-out-defines body) env))
(define (compound-procedure? p)
  (tagged-list? p 'procedure))
(define (procedure-params p) (cadr   p))
(define (procedure-body   p) (caddr  p))
(define (procedure-env    p) (cadddr p))

(define primitive-procedures
  (list (list 'car   car)
        (list 'cdr   cdr)
        (list 'cons  cons)
        (list 'null? null?)
        (list '+ +)
        (list '- -)
        (list '* *)
        (list '/ /)
        (list '= =)))
(define (primitive-procedure-names)
  (map car primitive-procedures))
(define (primitive-procedure-objects)
  (map (lambda (proc)
        (list 'primitive (cadr proc)))
       primitive-procedures))
(define (primitive-implementation proc)
  (cadr proc))

;; Environment ---------------------------------------------

(define (enclosing-environment env) (cdr env))
(define (first-frame env) (car env))
(define the-empty-environment '())

(define (make-frame variables values)
  (cons variables values))
(define (frame-variables frame) (car frame))
(define (frame-values frame) (cdr frame))
(define (add-binding-to-frame! var val frame)
  (set-car! frame (cons var (car frame)))
  (set-cdr! frame (cons val (cdr frame))))

(define (extend-environment vars vals base-env)
  (if (= (length vars) (length vals))
      (cons (make-frame vars vals) base-env)
      (if (< (length vars) (length vals))
          (error "Too many arguments supplied" vars vals)
          (error "Too few arguments supplied"
                 vars
                 vals))))

(define (lookup-variable-value var env)
  (define (env-loop env)
    (define (scan vars vals)
      (cond ((null? vars)
             (env-loop (enclosing-environment env)))
            ((eq? var (car vars))
             (if (eq? (car vals) '*unassigned*)
                 (error 'lookup "Unassigned val" var)
                 (car vals)))
            (else (scan (cdr vars)
                        (cdr vals)))))
    (if (eq? env the-empty-environment)
        (error 'lookup "Unbound variable" var)
        (let ((frame (first-frame env)))
          (scan (frame-variables frame)
                (frame-values frame)))))
  (env-loop env))

(define (set-variable-value! var val env)
  (define (env-loop env)
    (define (scan vars vals)
      (cond ((null? vars)
             (env-loop (enclosing-environment env)))
            ((eq? var (car vars))
             (set-car! vals val))
            (else (scan (cdr vars)
                        (cdr vals)))))
    (if (eq? env the-empty-environment)
        (error "Unbound variable: SET!" var)
        (let ((frame (first-frame env)))
          (scan (frame-variables frame)
                (frame-values frame)))))
  (env-loop env))

(define (define-variable! var val env)
  (let ((frame (first-frame env)))
    (define (scan vars vals)
      (cond ((null? vars)
             (add-binding-to-frame! var val frame))
            ((eq? var (car vars))
             (set-car! vals val))
            (else (scan (cdr vars) 
                        (cdr vals)))))
    (scan (frame-variables frame)
          (frame-values frame))))

(define (setup-environment)
  (let ((initial-env
         (extend-environment
          (primitive-procedure-names)
          (primitive-procedure-objects)
          the-empty-environment)))
    (define-variable! 'true  #t initial-env)
    (define-variable! 'false #f initial-env)
    initial-env))

(define the-global-environment (setup-environment))
(define global-env the-global-environment)

;; Lazy ----------------------------------------------------

(define (delay-it exp env)
  (list 'thunk exp env))
(define (thunk? obj) (tagged-list? obj 'thunk))
(define (thunk-exp thunk) (cadr thunk))
(define (thunk-env thunk) (caddr thunk))

(define (evaluated-thunk? obj)
  (tagged-list? obj 'evaluated-thunk))

(define (thunk-value evaluated-thunk) 
  (cadr evaluated-thunk))

(define (force-it obj)
  (cond ((thunk? obj)
         (let ((result
                (actual-value 
                 (thunk-exp obj)
                 (thunk-env obj))))
           (set-car! obj 'evaluated-thunk)
           ;; replace exp with its value:
           (set-car! (cdr obj) result) 
           ;; forget unneeded env:
           (set-cdr! (cdr obj) '()) 
           result))
        ((evaluated-thunk? obj)
         (thunk-value obj))
        (else obj)))

(define (actual-value exp env)
  (force-it (eval-expr exp env)))

(define (list-of-arg-values exps env)
  (if (null? exps)
      '()
      (cons (actual-value (car exps) env)
            (list-of-arg-values 
             (cdr exps)
             env))))

(define (list-of-delayed-args exps env)
  (if (null? exps)
      '()
      (cons (delay-it (car exps) env)
            (list-of-delayed-args 
             (cdr exps)
             env))))

;; Eval ----------------------------------------------------

(define (self-evaluating? expr)
  (cond ((number? expr) #t)
        ((string? expr) #t)
        ((eq? expr #t)  #t)
        ((eq? expr #f)  #t)
        (else #f)))

(define (quoted? exp) (tagged-list? exp 'quote))
(define (text-of-quotation exp) (cadr exp))

(define (variable? exp) (symbol? exp))

(define (assignment? exp) (tagged-list? exp 'set!))
(define (assignment-variable exp) (cadr exp))
(define (assignment-value exp) (caddr exp))
(define (eval-assignment exp env)
  (set-variable-value! 
   (assignment-variable exp)
   (eval-expr (assignment-value exp) env)
   env)
  'ok)

(define (lambda? exp) (tagged-list? exp 'lambda))
(define (lambda-parameters exp) (cadr exp))
(define (lambda-body exp) (cddr exp))
(define (make-lambda parameters body)
  (cons 'lambda (cons parameters body)))

(define (definition? exp) (tagged-list? exp 'define))
(define (definition-variable exp)
  (if (symbol? (cadr exp))
      (cadr exp)
      (caadr exp)))
(define (definition-value exp)
  (if (symbol? (cadr exp))
      (caddr exp)
      (make-lambda
       (cdadr exp)   ; formal parameters
       (cddr exp)))) ; body
(define (eval-definition exp env)
  (define-variable! 
    (definition-variable exp)
    (eval-expr (definition-value exp) env)
    env)
  'ok)

;; (define (eval-if exp env)
;;   (if (true? (eval-expr (if-predicate exp) env))
;;       (eval-expr (if-consequent  exp) env)
;;       (eval-expr (if-alternative exp) env)))

(define (eval-if exp env)
  (if (true? (actual-value (if-predicate exp) env))
      (eval-expr (if-consequent exp) env)
      (eval-expr (if-alternative exp) env)))

(define (begin? exp) (tagged-list? exp 'begin))
(define (begin-actions exp) (cdr exp))
(define (last-exp? seq) (null? (cdr seq)))
(define (first-exp seq) (car seq))
(define (rest-exps seq) (cdr seq))

(define (eval-sequence exps env)
  (cond ((last-exp? exps)
         (eval-expr (first-exp exps) env))
        (else
         (eval-expr (first-exp exps) env)
         (eval-sequence (rest-exps exps)
                        env))))

(define (application? exp) (pair? exp))
(define (operator exp) (car exp))
(define (operands exp) (cdr exp))
(define (list-of-values exps env)
  (if (null? exps)
      '()
      (cons (eval-expr (car exps) env)
            (list-of-values (cdr exps) env))))

(define (cond? exp) (tagged-list? exp 'cond))
(define (cond-clauses exp) (cdr exp))
(define (cond-else-clause? clause)
  (eq? (cond-predicate clause) 'else))
(define (cond-predicate clause) (car clause))
(define (cond-actions clause) (cdr clause))
(define (cond->if exp)
  (expand-clauses (cond-clauses exp)))
(define (sequence->exp seq)
  (cond ((null? seq) seq)
        ((last-exp? seq) (first-exp seq))
        (else (make-begin seq))))
(define (make-if predicate consequent alternative)
  (list 'if predicate consequent alternative))
(define (expand-clauses clauses)
  (if (null? clauses)
      'false     ; no else clause
      (let ((first (car clauses))
            (rest (cdr clauses)))
        (if (cond-else-clause? first)
            (if (null? rest)
                (sequence->exp (cond-actions first))
                (error "ELSE clause isn't last: COND->IF"
                       clauses))
            (make-if (cond-predicate first)
                     (sequence->exp (cond-actions first))
                     (expand-clauses rest))))))

(define (eval-expr exp env)
  (cond ((self-evaluating? exp) exp)
        ((variable?   exp) (lookup-variable-value exp env))
        ((quoted?     exp) (text-of-quotation exp))
        ((assignment? exp) (eval-assignment exp env))
        ((definition? exp) (eval-definition exp env))
        ((if? exp) (eval-if exp env))
        ((lambda? exp)
         (make-procedure
          (lambda-parameters exp)
          (lambda-body exp)
          env))
        ((begin? exp)
         (eval-sequence 
          (begin-actions exp) 
          env))
        ((cond? exp) (eval-expr (cond->if exp) env))
        ((let? exp) (eval-expr (let->combination exp) env))
        ((letrec? exp) (eval-expr (letrec->let exp) env))
        ;; ((application? exp)
        ;;  (apply-proc (eval-expr (operator exp) env)
        ;;              (list-of-values (operands exp) env)))
        ((application? exp)
         (apply-proc (actual-value (operator exp) env)
                     (operands exp)
                     env))
        (else
         (error "Unknown expression type: EVAL" exp))))

;; Apply ---------------------------------------------------

(define (primitive-procedure? proc)
  (tagged-list? proc 'primitive))

(define (apply-primitive-procedure proc args)
  (apply (primitive-implementation proc) args))

;; (define (apply-proc procedure args)
;;   (cond ((primitive-procedure? procedure)
;;          (apply-primitive-procedure procedure args))
;;         ((compound-procedure? procedure)
;;          (eval-sequence
;;            (procedure-body procedure)
;;            (extend-environment
;;              (procedure-params procedure)
;;              args
;;              (procedure-env procedure))))
;;         (else
;;          (error "Unknown procedure type: APPLY"
;;                 procedure))))

(define (apply-proc procedure args env)
  (cond ((primitive-procedure? procedure)
         (apply-primitive-procedure
          procedure
          (list-of-arg-values
           args
           env)))  ; changed
        ((compound-procedure? procedure)
         (eval-sequence
          (procedure-body procedure)
          (extend-environment
           (procedure-params procedure)
           (list-of-delayed-args
            args
            env)   ; changed
           (procedure-env procedure))))
        (else
         (error "Unknown procedure type: APPLY"
                procedure))))

;; Exercises -----------------------------------------------

;; 4.1 
(define (list-of-values exps env)
  (if (null? exps)
      '()
      (let ((left (eval-expr (car exps) env)))
        (let ((rest (list-of-values (cdr exps) env)))
          (cons left rest)))))

;; 4.6
(define (let? exp) (tagged-list? exp 'let))
(define (let->combination exp)
  (define params (map car (cadr exp)))
  (define args (map cadr (cadr exp)))
  (define body (cddr exp))
  (cons (make-lambda params body)
        args))

;; 4.16

(define (make-let vars vals body)
  (define (loop vars vals)
    (if (null? vars)
        '()
        (cons (list (car vars)
                    (car vals))
              (loop (cdr vars)
                    (cdr vals)))))
  (cons 'let (cons (loop vars vals) body)))

(define (make-new-body vars vals body)
  (define (loop vars vals)
    (if (null? vars)
        '()
        (cons (list 'set! (car vars) (car vals))
              (loop (cdr vars) (cdr vals)))))
  (append (loop vars vals) body))

(define (scan-out-defines body)
  (define vars (map cadr  (filter definition? body)))
  (define vals (map caddr (filter definition? body)))
  (define exps
    (filter (lambda (x) (not (definition? x))) body))
  (define new-body
    (make-new-body vars vals exps))
  (if (null? vars)
      body
      (list
        (make-let vars
                  (map (lambda (x) 
                        (quote '*unassigned*))
                       vals)
                  new-body))))

;; 4.18

;; The method shown in the exercise will not work because
;; both a and b will be evaluated in an environment where
;; y and dy are bound to '*unassigned*

;; The method from the text will work, because dy will be set
;; after y has been set to its initial value

;; 4.20

(define (letrec? exp) (tagged-list? exp 'letrec))
(define (letrec-vars exp) (map car  (cadr exp)))
(define (letrec-vals exp) (map cadr (cadr exp)))
(define (letrec-body exp) (cddr exp))
(define (letrec->let exp)
  (define vars (letrec-vars exp))
  (define vals (letrec-vals exp))
  (define body (letrec-body exp))
  (define new-body (make-new-body vars vals body))
  (make-let vars
            (map (lambda (x) (quote '*unassigned*)) vals)
            new-body))

;; 4.21

(define Y
  (lambda (F)
    ((lambda (x)
       (F (lambda (arg) ((x x) arg))))
     (lambda (x)
       (F (lambda (arg) ((x x) arg)))))))

(define fact-gen
  (lambda (f)
    (lambda (n)
      (if (= n 0)
          1
          (* n (f (- n 1)))))))
(define fact (Y fact-gen))

(define fib-gen
  (lambda (f)
    (lambda (n)
      (if (< n 2)
          n
          (+ (f (- n 1))
             (f (- n 2)))))))
(define fib (Y fib-gen))

;; Tests ---------------------------------------------------

;; (define (test-eval-env env exp expected)
;;   (define result (eval-expr exp env))
;;   (if (equal? result expected)
;;       (printf "Passed: (eval-expr ~s)~n" exp)
;;       (begin 
;;         (printf "Failed: ~s~n" exp)
;;         (printf "  Expected: ~s~n" expected)
;;         (printf "  Actual:   ~s~n" result))))

;; (define (test-eval exp expected)
;;   (test-eval-env (setup-environment) exp expected))

;; (define (run-tests)
;;   (define test-env (setup-environment))
;;   (define ex1
;;     '((lambda ()
;;         (define u 1)
;;         (define v 2)
;;         (cons u v))))
;;   (define ex2
;;     '(let ((u '*unassigned*) (v '*unassigned*))
;;       (set! u 1)
;;       (set! v 2)
;;       (cons u v)))
;;   (define ex3
;;     '((lambda (x)
;;         (letrec
;;           ((even?
;;             (lambda (n)
;;               (if (= n 0)
;;                   true
;;                   (odd? (- n 1)))))
;;           (odd?
;;             (lambda (n)
;;               (if (= n 0)
;;                   false
;;                   (even? (- n 1))))))
;;         (even? x))) 5))

;;   (printf "Expression tests ---------------------------~n")
;;   (test-eval '1 1)
;;   (test-eval '(quote a) 'a)
;;   (test-eval '"abc" "abc")
;;   (test-eval '(* 2 3) (* 2 3))
;;   (test-eval '(if 5  "true" "false") "true")
;;   (test-eval '(if #t "true" "false") "true")
;;   (test-eval '(if #f "true" "false") "false")
;;   (test-eval '(if true  "true" "false") "true")
;;   (test-eval '(if false "true" "false") "false")
;;   (test-eval '(cond (true  "true") (else "false")) "true")
;;   (test-eval '(cond (false "true") (else "false")) "false")
;;   (test-eval '(cons 1 2) (cons 1 2))
;;   (test-eval '(define x 1) 'ok)
;;   (test-eval '(define x (cons 1 2)) 'ok)
;;   (test-eval '(define f (lambda () 'result)) 'ok)
;;   (test-eval '(define f (lambda (x y) (cons x y))) 'ok)
;;   (test-eval '(define (f) 'result) 'ok)
;;   (test-eval '(define (f x y) (cons x y)) 'ok)
;;   (test-eval '(begin 'a 'b) 'b)
;;   (test-eval '(begin (define a 1) (define b 2)) 'ok)
;;   (test-eval '(let ((a 1) (b 2)) (cons a b)) (cons 1 2))
;;   (test-eval ex1 (cons 1 2))
;;   (test-eval ex2 (cons 1 2))
;;   (test-eval ex3 #f)

;;   (printf "~nEnvironment tests --------------------------~n")
;;   (test-eval-env test-env '(define (f x y) (cons x y)) 'ok)
;;   (test-eval-env test-env '(f 3 4) (cons 3 4))
;;   (test-eval-env test-env '(define x 1) 'ok)
;;   (test-eval-env test-env '(define y 2) 'ok)
;;   (test-eval-env test-env '(define z (cons x y)) 'ok)
;;   (test-eval-env test-env 'x 1)
;;   (test-eval-env test-env 'y 2)
;;   (test-eval-env test-env 'z (cons 1 2))
;;   (test-eval-env test-env '(define (try a b)
;;                             (if (= a 0) 1 b))
;;                           'ok)
;;   (test-eval-env test-env '(try 0 (/ 1 0)) 1))

;; (run-tests)