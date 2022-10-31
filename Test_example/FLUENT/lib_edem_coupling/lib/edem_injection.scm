;; Define (get-dpm-injections-list) if not already defined (as in 18.0)
(if (not (symbol-bound? 'get-dpm-injections-list (the-environment)))
    (define (get-dpm-injections-list)
      (rpgetvar 'dpm/injections-list)))


(define (get-edem-particle-type-name index)
  (let ((name (%edem-particle-type-name index)))
    (if name
        (join-string (split-string-whitespace (lowercase name)) "-")
        #f)))

(define (get-edem-material-name . args)
  (let ((index (if (pair? args) (car args) -1))
        (basic-name (rpgetvar 'edem/material-name)))
    (string->symbol
     (if (>= index 0)
         (let ((particle-name (get-edem-particle-type-name index)))
           (if particle-name
               (format #f "~a-~a" basic-name particle-name)
               (format #f "~a-~a" basic-name index)))
         basic-name))))

(define (edem-material-created? . args)
  (let ((index (if (pair? args) (car args) -1)))
    (member (get-edem-material-name index) (get-all-dpm-material-names 'inert))))

(define (change-edem-material-density . args)
  (let ((dens (car args))
        (index (if (pair? (cdr args)) (cadr args) -1)))
    (if  (edem-material-created? index)
         (begin
           (with-output-to-string
             (lambda ()
               (ti-menu-load-string
                (format #f "/define/material/change-create ~a , y constant ~a n"
                        (get-edem-material-name index)
                        dens
                        ))))
           (rpsetvar 'edem/material-density dens)
           dens))))

(define (change-edem-material-specific-heat . args)
  (let ((cp (car args))
        (index (if (pair? (cdr args)) (cadr args) -1)))
    (if  (edem-material-created? index)
         (begin
           (with-output-to-string
             (lambda ()
               (ti-menu-load-string
                (format #f "/define/material/change-create ~a , n y constant ~a"
                        (get-edem-material-name index)
                        cp
                        ))))
           (rpsetvar 'edem/material-specific-heat cp)
           cp))))


(define (get-edem-injection-name . args)
  (let ((index (if (pair? args) (car args) -1))
        (basic-name (rpgetvar 'edem/injection-name)))
    (string->symbol
     (if (>= index 0)
         (let ((particle-name (get-edem-particle-type-name index)))
           (if particle-name
               (format #f "~a-~a" basic-name particle-name)
               (format #f "~a-~a" basic-name index)))
         basic-name))))

(define (edem-injection-created? . args)
  (let ((index (if (pair?  args) (car args) -1)))
    (member (get-edem-injection-name index) (map car (get-dpm-injections-list)))))


(define (create-edem-material . args)
  (let ((index (if (pair?  args) (car args) -1)))
    (if (not (edem-material-created? index))
        (begin
          (if (not (member 'anthracite (get-all-dpm-material-names 'inert)))
              ;; Copy anthracite to list of materials from the data base
              (with-output-to-string
                (lambda ()
                  (ti-menu-load-string
                   (format #f "/define/material/copy inert-particle anthracite")))))

          ;; Now change anthracite to edem/material-name
          (if (member 'anthracite (get-all-dpm-material-names 'inert))
              (begin
                (with-output-to-string
                  (lambda ()
                    (ti-menu-load-string
                     (format #f "/define/material/change-create anthracite ~a y constant ~a y constant ~a y"
                             (get-edem-material-name index)
                             (rpgetvar 'edem/material-density)
                             (rpgetvar 'edem/material-specific-heat)
                             ))))
                (cx-info-dialog (format #f "The material \"~a\" has been created for the EDEM injection.\nPlease check assigned default property values like density and specific heat capacity" (get-edem-material-name index)))
                )
              (cx-info-dialog (format #f "Cannot copy particle material \"anthracite\" from data base"))))
        ()
        )))

(define (get-edem-injection . args)
  (let ((index (if (pair?  args) (car args) -1)))
    (assoc (get-edem-injection-name index) (get-dpm-injections-list))))

(define (set-edem-injection-material . args)
  (let ((index (if (pair?  args) (car args) -1)))
    (set-property-on-injections-list
     'material
     (get-edem-material-name index)
     (list (get-edem-injection index)))))

(define (create-edem-injection . args)
  (let ((index (if (pair?  args) (car args) -1)))
    (if (not (edem-injection-created? index))
        (begin
          (with-output-to-string
            (lambda ()
              (ti-menu-load-string
               (format #f "/define/injections/create-injection ~a" (get-edem-injection-name index)))))

          (create-edem-material index)
          (set-edem-injection-material index)
          (cx-info-dialog (format #f "Injection \"~a\" has been created for EDEM particles.\n" (get-edem-injection-name index)))
          (if (rpgetvar 'dpm/dpm_vof?)
              (set-edem-injection-phase index)
              ;; (cx-info-dialog (format #f "Please assign an appropriate Discrete Phase Domain\n"))
              )))))

(define (create-all-edem-injections num)
  (if (> num 0)
      (if (> num 1)
          (do ((index 0 (+ index 1)))
              ((>= index num) num)
            (create-edem-injection index))
          (create-edem-injection) ;; Single injection with basename and no index
          ))
  (delete-old-edem-injection)
  (delete-old-edem-material))

(define (delete-old-edem-injection)
  (if (and (edem-injection-created?) (edem-injection-created? 0))
      (delete-injection (get-edem-injection-name))))

(define (delete-old-edem-material)
  (if (and (edem-material-created?) (edem-material-created? 0))
      (delete-material (get-edem-material-name) #f)))
