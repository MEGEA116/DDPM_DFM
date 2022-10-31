;;
;; Shadow scheme functions.


(let-syntax ((shadow
              (if (cx-single?)
                  (macro syms
                    (cons 'begin
                          (map (lambda (sym)
                                 `(define ,sym
                                    (let ((fn ,sym))
                                      (lambda f
                                        (cx-single-send fn f)))))
                               syms)))
                  (macro syms
                    (cons 'begin
                          (map (lambda (sym)
                                 `(define ,sym
                                    (lambda f
                                      (cx-sendq (cons ',sym f)))))
                               syms))))))

  ;; Form this list from all user defined scheme subs

  (shadow
   %edem-is-connected?
   %edem-update-settings
   %edem-n-req-dpm-user-reals
   %edem-n-particle-types
   %edem-n-particle-types-look-ahead
   %edem-perform-num-analysis-steps
   %edem-particle-type-name
   )
)
