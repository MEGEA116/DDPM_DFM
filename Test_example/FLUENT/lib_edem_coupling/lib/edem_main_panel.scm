

;; Panel for EDEM coupling

(define (list-index pred list)
  (let lp ((l list) (i 0))
    (cond ((null? l) #f)
          ((equal? (car l) pred) i)
          (else (lp (cdr l) (1+ i))))))

(define (set-edem-dpm-reals)
  (let ((n-dpm-reals (%edem-n-req-dpm-user-reals)))
    (set-min-dpm-reals n-dpm-reals)
    ))

(define (set-dpm-parameters)
  ;; Disable accuracy control and automated tracking scheme selection
  (rpsetvar 'dpm/auto-step-size-control? #f)
  (rpsetvar 'dpm/auto-tracking-scheme-selection? #f)

  ;; Disable flushing of dpm sources at beginning of time step
  (rpsetvar 'dpm/flush-sources-at-timestep? #f)

  ;; Set dpm under relaxation factors to 1.0 for coupled simulations
  (if (sg-dpm?)
      (begin
        (rpsetvar 'dpm/relax 1.0)
        (if (rp-var-object 'dpm/pseudo-relax) (rpsetvar 'dpm/pseudo-relax 1.0))
        ))

  ;; Specify implicit tracking scheme
  (let ((implicit-scheme (cdr (assv 'implicit (append tracking-schemes-lower tracking-schemes-higher)))))
    (if (not (= implicit-scheme (rpgetvar 'dpm/tracking-scheme)))
        (rpsetvar 'dpm/tracking-scheme implicit-scheme)))

  ;; Enable interaction
  (switch-cphase-interaction #t #f)
  (dpm-parameters-changed)
  )

(define (edem-n-particle-types)
  (let ((n-particle-types (%edem-n-particle-types)))
    (if (> n-particle-types 0)
        n-particle-types               
        (%edem-n-particle-types-look-ahead)
        )))

(define (set-up-edem-injections)
  (let ((n-particle-types (edem-n-particle-types)))
    (if (> n-particle-types 0)
        (begin
          (create-all-edem-injections n-particle-types)
          (if (and (rpgetvar 'edem/use-ddpm?) (edem-phase-valid?))
                    (set-all-edem-injection-phases n-particle-types))
          #t)
        #f)))

(define (set-dpm-niter)
  (let ((prev-dpm-niter (rpgetvar 'dpm/niter))
        (big-dpm-niter 1000000))
    (if (not (= prev-dpm-niter big-dpm-niter))
        (rpsetvar 'edem/previous-dpm-niter prev-dpm-niter)
        )
    (if (or (rpgetvar 'edem/use-fluent-drag?) (and (rf-energy?) (rpgetvar 'edem/use-fluent-heat-transfer?)))
        (rpsetvar 'dpm/niter big-dpm-niter)  ;; If drag or heat calculated by Fluent, override user's selection for dpm/niter
        (rpsetvar 'dpm/niter (rpgetvar 'edem/previous-dpm-niter))
        )
    (dpm-parameters-changed);; Let the solver know about changed dpm parameter 'dpm/niter
    (cx-changed 'dpm-panel) ;; Need to update an open DPM panel
    )
  )

(define (check-udf-hooks)
      (if (or (rpgetvar 'edem/use-fluent-drag?) (rpgetvar 'edem/use-fluent-heat-transfer?))
          (begin
            (remove-adjust-udf "adjust_edem_solution::lib_edem_coupling")
            (remove-execute-at-end-udf "update_edem_at_end::lib_edem_coupling")
            )
          (begin
            (add-adjust-udf "adjust_edem_solution::lib_edem_coupling")
            (add-execute-at-end-udf "update_edem_at_end::lib_edem_coupling")
            )
          )
      (add-execute-at-exit-udf "disconnect_edem_coupling_at_exit::lib_edem_coupling")
      )

(define edem-convection-model-options
  (list "None" "Ranz Marshall") ;; 0:"None" 1:"Ranz Marshall"
)

(define edem-radiation-model-options
  (list "None" "Emission Only") ;; 0:"None" 1:"Emission Only"
)

(define (edem-is-connected?)
  (and (cx-client?)
       (case-valid?)
       (%edem-is-connected?)))

(define gui-edem-coupling
  (let ((panel #f)
	(remote-host?)
	(host-ip-address)
    (connect)
    (disconnect)
    (synchronize)
    (status-message)
    (convection-selector)
    (radiation-selector)
    (use-fluent-drag?)
    (scale-up-factor)
    (thermal-frame)
    (use-fluent-heat-transfer?)
    (use-multiphase?)
    (use-ddpm?)
	)


;; Standard panel callbacks

    (define (update-cb . args)		;; Update panel fields
      (cx-set-toggle-button remote-host? (rpgetvar 'edem/remote-host?))
      (cx-set-toggle-button use-fluent-drag? (rpgetvar 'edem/use-fluent-drag?))
      (cx-set-real-entry scale-up-factor (rpgetvar 'edem/scale-up-factor))
      (cx-set-toggle-button use-fluent-heat-transfer? (rpgetvar 'edem/use-fluent-heat-transfer?))
      (cx-set-toggle-button use-ddpm? (rpgetvar 'edem/use-ddpm?))
      (cx-enable-item convection-selector (not (rpgetvar 'edem/use-fluent-heat-transfer?)))
      (cx-enable-item use-multiphase? (not (edem-phase-valid?)))
      (cx-enable-item use-ddpm? (edem-phase-valid?))
      (cx-set-text-entry host-ip-address (rpgetvar 'edem/host-ip-address))
      (cx-enable-item host-ip-address (rpgetvar 'edem/remote-host?))
      

      (let ((connected? (edem-is-connected?)))

        (cx-enable-item connect (not connected?))
        (cx-enable-item disconnect connected?)
        (cx-enable-item synchronize connected?)

        (cx-set-text-entry status-message (if connected?
                                              (if (rpgetvar 'edem/remote-host?) 
                                                  "Connected Remotely"
                                                  "Connected")
                                              "Not Connected")
                           ))

      (cx-set-list-selections convection-selector (list (rpgetvar 'edem/convective-heat-option)))
      (cx-set-list-selections radiation-selector (list (rpgetvar 'edem/radiative-heat-option)))

      (cx-enable-tab-button thermal-frame (rf-energy?))
      (set-dpm-parameters)
      (set-dpm-niter)
      (set-edem-dpm-reals)
      )

    (define (save-edem-settings .args)
      (rpsetvar 'edem/remote-host? (cx-show-toggle-button remote-host?))
      (rpsetvar 'edem/host-ip-address (if (cx-show-toggle-button remote-host?)
                                          (cx-show-text-entry host-ip-address)
                                          (rp-var-default 'edem/host-ip-address)))

      (rpsetvar 'edem/use-fluent-drag? (cx-show-toggle-button use-fluent-drag?))
      (rpsetvar 'edem/scale-up-factor (cx-show-real-entry scale-up-factor))
      (rpsetvar 'edem/use-fluent-heat-transfer? (cx-show-toggle-button use-fluent-heat-transfer?))
      )

    (define (apply-cb . args)
      (save-edem-settings)
      (set-dpm-parameters)
      (set-dpm-niter)
      (check-udf-hooks)

      ;; Check that sufficient dpm scalars are specified
      (set-edem-dpm-reals)
      (%edem-update-settings)  ;; Send changed information to C-side

      (cx-changed 'models-taskpage) ;; Update EDEM coupling status in models taskpage
      )

;; Panel specific callbacks

    (define (remote-host-cb . args)
      (cx-enable-item host-ip-address (cx-show-toggle-button remote-host?))
      )

    (define (connect-cb . args)
      (if (rp-unsteady?)
          (begin
            (cx-set-text-entry status-message "Connecting..." )
            (set-edem-dpm-reals)
            (rpsetvar 'edem/remote-host? (cx-show-toggle-button remote-host?))
            (rpsetvar 'edem/host-ip-address (if (cx-show-toggle-button remote-host?)
                                                (cx-show-text-entry host-ip-address)
                                                (rp-var-default 'edem/host-ip-address)))

            (%udf-on-demand "connect_edem_coupling::lib_edem_coupling")


            (let ((connected? (edem-is-connected?)))
              (cx-enable-item connect (not connected?))
              (cx-enable-item disconnect connected?)
              (cx-enable-item synchronize connected?)
              (if (not connected?)
                  (let ((err-msg (format #f "Unable to connect to EDEM.\n Check the EDEM coupling server has been started~a" 
                                         (if (rpgetvar 'edem/remote-host?)
                                             (format #f "\n  on the Remote Host : ~a." (rpgetvar 'edem/host-ip-address))
                                             "."))))
                    (cx-warning-dialog err-msg)
                    (format #t "\nERROR: ~a\n" err-msg)
                    )
                  )

              (cx-set-text-entry status-message (if connected?
                                                    (if (rpgetvar 'edem/remote-host?) 
                                                        "Connected Remotely"
                                                        "Connected")
                                                    "Failed to Connect")
                                 )

              (rpsetvar 'edem/connected? connected?) ;; Used to check status in Adjust UDF after case loaded

              (if connected?  ;; Set up injections and function hooks
                  (begin
                    (if (not (set-up-edem-injections))
                        (add-execute-command 'update-edem-command "(update-edem-command)" 1 #t #f #t)) ;; No injections made yet so update EDEM by using an execute command
                    (check-udf-hooks)
                    ))
              )
            
            (cx-changed 'models-taskpage) ;; Update EDEM coupling status in models taskpage
            )
          (let ((err-msg "Switch to the Transient solver\n  before connecting to EDEM."))
            (format #t "\n\nWARNING: ~a\n" err-msg)
            (cx-warning-dialog (format #f err-msg))
            (save-edem-settings) ;; Save changes made so far to RP vars so not lost after (models-changed)
            )
          ))

    (define (disconnect-cb . args)
      (cx-set-text-entry status-message "Disconnecting..." )
      (%udf-on-demand "disconnect_edem_coupling::lib_edem_coupling")

      (let ((connected? (%edem-is-connected?))) ;; Use raw C command to force disconnect even if case not valid
        (cx-enable-item connect (not connected?))
        (cx-enable-item disconnect connected?)
        (cx-enable-item synchronize connected?)
        (if connected?
            (cx-warning-dialog (format #f "Unable to disconnect from EDEM~a" 
                                       (if (cx-show-toggle-button remote-host?)
                                           "\n on Remote Host."
                                           "."))))

        (cx-set-text-entry status-message (if connected?
                                              (if (rpgetvar 'edem/remote-host?) 
                                                  "Still Connected Remotely"
                                                  "Still Connected")
                                              "Not Connected"))

        (rpsetvar 'edem/connected? connected?) ;; Used to check status in Adjust UDF after case loaded
        )

      (cx-changed 'models-taskpage) ;; Update EDEM coupling status in models taskpage
      )

      (define (synchronize-cb . args)
        (%udf-on-demand "synchronize_fluent_to_edem_time::lib_edem_coupling"))

      (define (use-fluent-drag-cb . args)
        (rpsetvar 'edem/use-fluent-drag? (cx-show-toggle-button use-fluent-drag?))
        (set-dpm-niter)
        (check-udf-hooks)
        )

      (define (use-fluent-heat-transfer-cb . args)
        (rpsetvar 'edem/use-fluent-heat-transfer? (cx-show-toggle-button use-fluent-heat-transfer?))
        (cx-enable-item convection-selector (not (rpgetvar 'edem/use-fluent-heat-transfer?)))
        (set-dpm-niter)
        (check-udf-hooks)
        (set-edem-dpm-reals)
        )

      (define (use-multiphase-cb . args)
        (if (rp-unsteady?)
            (if (edem-is-connected?)  ;; Connection needed for setting up injections & function hooks
                (begin
                  (if (and (not (edem-phase-valid?))
                           (cx-ok-cancel-dialog (format #f "This will ~acreate a new Discrete Phase.\n\nThe solution will need to be initialised or a data file read in." (if (eq? sg-mphase? 'multi-fluid) "" "Switch on the Eulerian Muliphase model\n and "))))
                      (create-edem-phase))

                  (if (edem-phase-valid?)
                      (begin
                        (cx-enable-item use-ddpm? #t)
                        (cx-set-toggle-button use-ddpm? #t)
                        (cx-enable-item use-multiphase? #f)
                        (rpsetvar 'edem/use-ddpm? #t)
                        (apply-cb)) ;; Can't go back now so apply all changes so far
                      (begin
                        (cx-set-toggle-button use-ddpm? #f)
                        (cx-enable-item use-ddpm? #f)
                        (cx-enable-item use-multiphase? #t)
                        (rpsetvar 'edem/use-ddpm? #f)            ))


                  (begin
                    (if (not (set-up-edem-injections))
                        (add-execute-command 'update-edem-command "(update-edem-command)" 1 #t #f #t)) ;; No injections made yet so update EDEM by using an execute command
                    (check-udf-hooks)))

                (let ((err-msg "Connect to an EDEM process\n  before selecting Multiphase Coupling."))
                  (format #t "\n\nWARNING: ~a\n" err-msg)
                  (cx-warning-dialog (format #f err-msg))))

            (let ((err-msg "Switch to the Transient solver\n  before selecting Multiphase Coupling."))
              (format #t "\n\nWARNING: ~a\n" err-msg)
              (cx-warning-dialog (format #f err-msg)))))

      (define (use-ddpm-cb . args)
        (rpsetvar 'edem/use-ddpm? (cx-show-toggle-button use-ddpm?))
        (set-all-edem-injection-phases (edem-n-particle-types)))


      (define (convection-cb . args)
        (rpsetvar 'edem/convective-heat-option (list-index (car (cx-show-list-selections convection-selector)) edem-convection-model-options ))
        (set-edem-dpm-reals))

      (define (radiation-cb . args)
        (rpsetvar 'edem/radiative-heat-option (list-index (car (cx-show-list-selections radiation-selector)) edem-radiation-model-options ))
        (set-edem-dpm-reals))


      (lambda args
        (if (not panel)
            (let ((tabbed-frame)
                  (connection-frame)
                  (interaction-frame)
                  (connection-table)
                  (drag-frame)
                  (multiphase-frame)
                  (drag-table)
                  (thermal-table)
                  (multiphase-table)
                  )

              (set! panel (cx-create-panel "UNINSIM Compiled"
                                           apply-cb update-cb))

              (cx-add-dependent 'models panel update-cb)

              (set! tabbed-frame (cx-create-frame panel #f 'tabbed #t 'border #t))

              (set! connection-frame (cx-create-frame tabbed-frame "Connection"
                                                      'border #t))

              (set! interaction-frame (cx-create-frame tabbed-frame "Interaction"
                                                'tabbed #t
                                                'border #t))

              (set! drag-frame (cx-create-frame interaction-frame "Drag"
                                                'border #f))

              (set! thermal-frame (cx-create-frame interaction-frame "Thermal"
                                                   'border #f))

              (set! multiphase-frame (cx-create-frame interaction-frame "Multiphase"
                                                   'border #f))

              (set! connection-table (cx-create-table connection-frame ""
                                                      'border #f
                                                      'below 0
                                                      'right-of 0))


              (set! remote-host? (cx-create-toggle-button connection-table "EDEM Running on Remote Host  "
                                                          'activate-callback remote-host-cb
                                                          'table-options '(top-justify left-justify)
                                                          'row 0
                                                          'col 0
                                                          'col-span 2
                                                          ))

              (set! host-ip-address (cx-create-text-entry connection-table 
                                                          "Remote Host IP Address"
                                                          'width 18
                                                          'row 1
                                                          'col 0
                                                          'col-span 2
                                                          ))

              (set! status-message (cx-create-text-entry connection-table 
                                                         "Connection Status"
                                                         'width 18
                                                         'row 2
                                                         'col 0
                                                         'col-span 2
                                                         ))

              (cx-set-text-entry-editable status-message #f)

              (set! connect (cx-create-button connection-table "Connect" 
                                              'activate-callback connect-cb
                                              'row 3
                                              'col 0))

              (set! disconnect (cx-create-button connection-table "Disconnect"
                                                 'activate-callback disconnect-cb
                                                 'row 3
                                                 'col 1))

              (set! synchronize (cx-create-button connection-table "Synchronize to EDEM Time"
                                                 'activate-callback synchronize-cb
                                                 'row 4
                                                 'col 0
                                                 'col-span 2))


              (set! drag-table (cx-create-table drag-frame ""
                                                'border #f
                                                'below 0
                                                'right-of 0))

              (set! use-fluent-drag? (cx-create-toggle-button drag-table "Use Fluent Drag Law"
                                                              'activate-callback use-fluent-drag-cb
                                                              'table-options '(top-justify left-justify)
                                                              'row 0
                                                              'col 0
                                                              ))

              (set! scale-up-factor (cx-create-real-entry drag-table "Particle Scale-Up Factor"
                                                              'table-options '(top-justify left-justify)
                                                              'row 1
                                                              'col 0
                                                              ))

              (set! thermal-table (cx-create-table thermal-frame ""
                                                   'border #f
                                                   'below 0
                                                   'right-of 0))

              (set! convection-selector (cx-create-drop-down-list thermal-table "Convection"
                                                                  'activate-callback convection-cb
                                                                  'row 0
                                                                  'col 0
                                                                  'width 18))

              (cx-set-symbol-list-items convection-selector edem-convection-model-options)

              (set! radiation-selector (cx-create-drop-down-list thermal-table "Radiation"
                                                                 'activate-callback radiation-cb
                                                                 'row 1
                                                                 'col 0
                                                                 'width 18))

              (cx-set-symbol-list-items radiation-selector edem-radiation-model-options)

              (set! use-fluent-heat-transfer? (cx-create-toggle-button thermal-table "Use Fluent Heat Transfer"
                                                                       'activate-callback use-fluent-heat-transfer-cb
                                                                       'table-options '(top-justify left-justify)
                                                                       'row 2
                                                                       'col 0
                                                                       ))

              (set! multiphase-table (cx-create-table multiphase-frame ""
                                                      'border #f
                                                      'below 0
                                                      'right-of 0))

              (set! use-multiphase? (cx-create-button multiphase-table "Couple with Multiphase" 
                                              'activate-callback use-multiphase-cb
                                              'row 0
                                              'col 0))

              (set! use-ddpm? (cx-create-toggle-button multiphase-table "Use DDPM"
                                                              'activate-callback use-ddpm-cb
                                                              'table-options '(top-justify left-justify)
                                                              'row 1
                                                              'col 0
                                                              ))

              ))

        (cx-show-panel panel)
        )))


(define (update-edem-command)
  (if (edem-is-connected?)
      (if (set-up-edem-injections)
          (remove-execute-command 'update-edem-command)
          (%udf-on-demand "update_edem_solution::lib_edem_coupling")
          )))


;; Insert the EDEM coupling panel into the Models menu.


(in-package models-package
  (define edem-coupling (instance generic-model))
  (in-package edem-coupling
    (define-method (model-name) "EDEM Coupling") ;; Name as seen in the menu
    (define-method (get-model-setting)
      (if (edem-is-connected?)
          (if (rpgetvar 'edem/remote-host?) 
              "Connected Remotely" 
              "Connected")
          "Not Connected"))
    (define-method (callback) (gui-edem-coupling)) ;; Panel name
    ))

(register-model-in-models-taskpage 'edem-coupling)

