;; Define a new function to help add new variables but not overwrite current values:

(define (make-new-rpvar name default type)
  (if (not (rp-var-object name))
      (rp-var-define name default type #f)))

(define (set-min-dpm-reals n-dpm-reals)
  (if (> n-dpm-reals (rpgetvar 'udf/dpm/n-reals))
      (begin
        (rpsetvar 'udf/dpm/n-reals n-dpm-reals)
        (dpm-parameters-changed))))



(make-new-rpvar 'edem/connected? #f 'boolean)
(make-new-rpvar 'edem/time 0.0 'real) 

;; Set default EDEM host to local host
(make-new-rpvar 'edem/remote-host? #f 'boolean)
(make-new-rpvar 'edem/host-ip-address "127.0.0.1" 'string) 

(make-new-rpvar 'edem/injection-name "edem-injection" 'string) 
(make-new-rpvar 'edem/material-name "edem-material" 'string) 
(make-new-rpvar 'edem/material-density 1000.0 'real) 
(make-new-rpvar 'edem/material-specific-heat 4200 'real)
(make-new-rpvar 'edem/use-ddpm? #f 'boolean) 
(make-new-rpvar 'edem/phase-name "edem-phase" 'string) 
(make-new-rpvar 'edem/phase-id 0 'integer) 

(make-new-rpvar 'edem/use-fluent-drag? #t 'boolean)
(make-new-rpvar 'edem/scale-up-factor 1.0 'real)
(make-new-rpvar 'edem/use-fluent-heat-transfer? #t 'boolean)
(make-new-rpvar 'edem/convective-heat-option 0 'integer)
(make-new-rpvar 'edem/radiative-heat-option 0 'integer)
(make-new-rpvar 'edem/previous-dpm-niter 10 'integer)

(make-new-rpvar 'edem/n-cross-sections 100 'integer)
(make-new-rpvar 'edem/n-cross-section-samples 1000 'integer)
(make-new-rpvar 'edem/n-surface-area-samples 1000 'integer)


