(define (split-string str . args)
  ;; Return a list of strings by splitting the input string at positions of <char> (or first char of string)
  ;;  or whitespace if char not set.

  (let ((splitter (if (pair? args) (car args) #f)))
    (if splitter
        (let ((split-char (if (string? splitter) (car (string->list splitter)) splitter)))
          (let loop1 ((s-as-l (string->list str)))
            (let loop2 ((lst s-as-l)
                        (result '()))
              (if (null? lst)
                  (list (list->string (reverse result)))
                  (if (char=? (car lst) split-char)
                      (cons (list->string (reverse result)) (loop1 (cdr lst)))
                      (loop2 (cdr lst) (cons (car lst) result))
                      )))))
        (split-string-whitespace str)
        )))

(define (split-string-whitespace str)
  ;; Return a list of strings by splitting the input string at any block of whitespace

  (let loop1 ((s-as-l (string->list str)))
    (let loop2 ((lst s-as-l)
                (result '()))
      (if (null? lst)
          (list (list->string (reverse result)))
          (if (char-whitespace? (car lst))
              (if (null? result)
                  (loop1 (cdr lst))
                  (cons (list->string (reverse result)) (loop1 (cdr lst))))
              (loop2 (cdr lst) (cons (car lst) result))
              )))))

(define (join-string list-strings join-str)
  (if (null? list-strings)
      ""
      (let ((joined-str (car list-strings)))
        (do ((str-list (cdr list-strings) (cdr str-list)))
            ((null? str-list) joined-str)
          (set! joined-str (string-append joined-str join-str (car str-list)))
          ))))

