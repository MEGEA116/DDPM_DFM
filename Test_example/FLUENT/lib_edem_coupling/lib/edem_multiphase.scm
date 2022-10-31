
(define (edem-phase-valid?)
  (and (eq? (sg-mphase?) 'multi-fluid) (member (string->symbol (rpgetvar 'edem/phase-name)) (map domain-name (get-phase-domains))))
)

(define (create-edem-phase)
  (if (not (sg-mphase?))
      (ti-menu-load-string "/define/models/multiphase/model eulerian ")
      )

  
  (if (not (edem-phase-valid?))
      (let* ((parent-domain (get-domain parent-domain-id))
             (phase-name (rpgetvar 'edem/phase-name))
             (phase-id (apply max (map domain-id (get-phase-domains)))))
        (ti-menu-load-string (format #f "/define/models/multiphase/eulerian-parameters y ")) ;; Use DDPM
        (ti-menu-load-string (format #f "/define/phases/phase-domain ~a ~a no no , , " phase-id phase-name))

        (set-data-valid #f) ;; New phase will make other single phase or multiphase data invalid

        (rpsetvar 'edem/phase-id phase-id)
        )))

(define (set-edem-injection-phase . args)
  (let* ((index (if (pair? args) (car args) -1))
        (edem-injection (get-edem-injection index)))
    (if edem-injection
        (let ((edem-injection-name (car edem-injection))
              (injection-phase-name (cdr (assq 'dpm-domain (cadr edem-injection))))
              (edem-phase-name (rpgetvar 'edem/phase-name)))
          (if (and (rpgetvar 'edem/use-ddpm?) (eq? (sg-mphase?) 'multi-fluid))
              (if (not (eq? (string->symbol edem-phase-name) injection-phase-name))
                  (ti-menu-load-string (format #f "/define/injections/set-injection-properties ~a , no no no no no no no no yes ~a , , , , , , , , , , " edem-injection-name edem-phase-name))
                  edem-phase-name)
              (if (not (eq? 'none injection-phase-name))
                  (ti-menu-load-string (format #f "/define/injections/set-injection-properties ~a , no no no no no no no no yes none , , , , , , , , , " edem-injection-name))
                  'none)
              )))))

(define (set-all-edem-injection-phases num)
  (if (> num 0)
      (if (> num 1)
          (do ((index 0 (+ index 1)))
              ((>= index num) num)
            (set-edem-injection-phase index))
          (set-edem-injection-phase) ;; Single injection with basename and no index
          )))

