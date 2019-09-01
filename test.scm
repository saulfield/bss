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

;