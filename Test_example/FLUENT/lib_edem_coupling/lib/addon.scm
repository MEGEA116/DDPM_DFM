;; Parallel EDEM CFD Coupling for ANSYS FLUENT - Version 2.2
;; Copyright 2015 ANSYS, Inc.     

;; addon.scm can be used to set up scheme vars and 
;; load other scheme files for panels for a UDF



(define (list-ref-def . args)  ;; Takes negative index to count from end
  (let ((l (car args))
        (l-len (length (car args)))
        (n (cadr args)))
    (if (and (< n l-len) (<= (- n) l-len)) ;; Index in bounds
        (car (list-tail l (if (< n 0) (+ l-len n) n)))
        (caddr args) ;; Optional default if index out of bounds
        )))


(define (load-addon-file filename)
  (load (cx-convert-consistent-separator (string-append (list-ref-def (rpgetvar 'udf/libname) -1) "/lib/" filename)) user-initial-environment)
  )


(load-addon-file "edem_bind_subrs.scm")  
(load-addon-file "edem_vars.scm")
(load-addon-file "edem_injection.scm")
(load-addon-file "edem_multiphase.scm")
(load-addon-file "edem_main_panel.scm")
(load-addon-file "edem_tui_commands.scm")
(load-addon-file "function_hooks.scm")
(load-addon-file "split_join_string.scm")

