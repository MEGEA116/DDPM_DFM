(define (ti-edem-connected?)
  (let ((connected? (edem-is-connected?))
        (host-msg (if (rpgetvar 'edem/remote-host?)
                      (format #f " on the Remote Host : ~a." (rpgetvar 'edem/host-ip-address))
                      ".")))

    (if connected?
        (format "\nConnected to an EDEM process~a\n" host-msg)
        (format "\nNot Connected to an EDEM process\n")
        )

    (rpsetvar 'edem/connected? connected?) ;; Used to check status in Adjust UDF after case loaded
    ))

(define (ti-edem-connect)
  (if (rp-unsteady?)
      (begin

        (set-edem-dpm-reals)
        (rpsetvar 'edem/remote-host? (y-or-n? (format #f "Remote Host?") (rpgetvar 'edem/remote-host?)))
        (rpsetvar 'edem/host-ip-address (if (rpgetvar 'edem/remote-host?)
                                            (ti-read-unquoted-string "IP Address" (rpgetvar 'edem/host-ip-address))
                                            (rp-var-default 'edem/host-ip-address)))

        (format #t "Connecting..." )
        (%udf-on-demand "connect_edem_coupling::lib_edem_coupling")

        (let ((connected? (edem-is-connected?))
              (host-msg (if (rpgetvar 'edem/remote-host?)
                            (format #f " on the Remote Host : ~a." (rpgetvar 'edem/host-ip-address))
                            ".")))

          (rpsetvar 'edem/connected? connected?)

          (if connected?
              (format "Connected~a\n" host-msg)
              (format "\nERROR: Unable to connect to EDEM.\n Check the EDEM coupling server has been started~a\n" host-msg)
              )

          (if connected?  ;; Set up injections, dpm parameters and UDF function hooks
              (begin
                (if (not (set-up-edem-injections))
                    (add-execute-command 'update-edem-command "(update-edem-command)" 1 #t #f #t)) ;; No injections made yet so update EDEM by using an execute command
                (set-dpm-parameters)
                (set-dpm-niter)
                (set-edem-dpm-reals)
                (%edem-update-settings)  ;; Send changed information to C-side
                (check-udf-hooks)
                ))
          )

	    (cx-changed 'models-taskpage) ;; Update EDEM coupling status in models taskpage
	    )

      (let ((err-msg "Switch to the Transient solver\n  before connecting to EDEM."))
        (format #t "\nWARNING: ~a\n" err-msg)
        )
      ))

(define (ti-edem-disconnect)

        (%udf-on-demand "disconnect_edem_coupling::lib_edem_coupling")

        (let ((connected? (%edem-is-connected?)) ;; Use raw C command to force disconnect even if case not valid
              (host-msg (if (rpgetvar 'edem/remote-host?)
                            (format #f " on the Remote Host : ~a.\n" (rpgetvar 'edem/host-ip-address))
                            ".\n")))
          (if connected?
              (format #t "Unable to disconnect from EDEM~a" host-msg)
              (format #t "Disconnected from EDEM~a" host-msg)
              )

          (rpsetvar 'edem/connected? connected?) ;; Used to check status in Adjust UDF after case loaded
          )


        (cx-changed 'models-taskpage) ;; Update EDEM coupling status in models taskpage
        )

(define (ti-edem-synchronize)
  (%udf-on-demand "synchronize_fluent_to_edem_time::lib_edem_coupling"))

(define (ti-use-fluent-drag)
  (rpsetvar 'edem/use-fluent-drag? (y-or-n? (format #f "Use Fluent Drag Law?") (rpgetvar 'edem/use-fluent-drag?)))
  (set-dpm-niter)
  (check-udf-hooks)
  )

(define (ti-use-fluent-heat-transfer)
  (rpsetvar 'edem/use-fluent-heat-transfer? (y-or-n? (format #f "Use Fluent Heat Transfer?") (rpgetvar 'edem/use-fluent-heat-transfer?)))
  (set-dpm-niter)
  (check-udf-hooks)
  (set-edem-dpm-reals)
  )

(define (get-model-option model-options default-value)
  (do ((i 0 (+ i 1))
       (models model-options (cdr models)))
      ((null? models) (let ((ret-i (read-integer "\nEnter model number" default-value)))
                        (if (and (>= ret-i 0) (< ret-i (length model-options)))
                            ret-i
                            default-value)))
    (if (= i default-value)
        (format "*~3a : ~a\n" i (car models))
        (format " ~3a : ~a\n" i (car models))
        )
    ))

(define (ti-edem-convection)
  (format "\nConvection model options:\n")
  (rpsetvar 'edem/convective-heat-option (get-model-option edem-convection-model-options (rpgetvar 'edem/convective-heat-option)))
  (set-edem-dpm-reals)
  (format "~a\n" (list-ref edem-convection-model-options (rpgetvar 'edem/convective-heat-option)))
  )

(define (ti-edem-radiation)
  (format "\nRadiation model options:\n")
  (rpsetvar 'edem/radiative-heat-option (get-model-option edem-radiation-model-options (rpgetvar 'edem/radiative-heat-option)))
  (set-edem-dpm-reals)
  (format "~a\n" (list-ref edem-radiation-model-options (rpgetvar 'edem/radiative-heat-option)))
  )

(define (ti-use-ddpm)
        (rpsetvar 'edem/use-ddpm? (y-or-n? "Use Dense Discrete Phase Model DDPM? " (rpgetvar 'edem/use-ddpm?)))
)

(define (ti-edem-multiphase)
  (if (rp-unsteady?)
      (begin
        (create-all-edem-injections (edem-n-particle-types))

        (if (and (not (edem-phase-valid?)) (y-or-n? (format #f "This will ~acreate a new Discrete Phase." (if (eq? sg-mphase? 'multi-fluid) "" "Switch on Eulerian Muliphase\nand ")) (rpgetvar 'edem/use-ddpm?)))
            (create-edem-phase)
            )

        (if (edem-phase-valid?)
            (begin
              (ti-use-ddpm)
              ))

        (if (edem-is-connected?)  ;; Set up injections & function hooks
            (begin
              (if (not (set-up-edem-injections))
                  (add-execute-command 'update-edem-command "(update-edem-command)" 1 #t #f #t)) ;; No injections made yet so update EDEM by using an execute command
              (check-udf-hooks)
              )))
      (let ((err-msg "Switch to the Transient solver\n  before selecting Multiphase EDEM Coupling."))
        (format #t "\n\nWARNING: ~a\n" err-msg)
        ))
  )

(define ti-edem-interaction-menu
  (make-menu
   "interaction"

   ("use-fluent-drag-law?"
    #t
    ti-use-fluent-drag
    "Option to use Fluent's standard drag law.")

   ("use-fluent-heat-transfer?"
    #t
    ti-use-fluent-heat-transfer
    "Option to use Fluent's standard heat transfer model.")

   ("convective-heat-option"
    #t
    ti-edem-convection
    "Select the model for convective heat transfer.")

   ("radiative-heat-option"
    #t
    ti-edem-radiation
    "Select the model for radiative heat transfer.")

   ("couple-with-multiphase"
    (not (edem-phase-valid?))
    ti-edem-multiphase
    "Connect to a local or remote EDEM process.")

   ("use-ddpm-multiphase-coupling?"
    (edem-phase-valid?)
    ti-use-ddpm
    "Use Fluent's Dense Discrete Particle Model for coupling.")
   ))

(define ti-edem-coupling-menu
  (make-menu
   "edem-coupling"

   ("interaction-settings/"
    #t
    ti-edem-interaction-menu
    "Additional EDEM interaction settings.")

   ("edem-connected?"
    #t
    ti-edem-connected?
    "Check whether Fluent is connected to a local or remote EDEM process.")

   ("connect"
    (not (edem-is-connected?))
    ti-edem-connect
    "Connect to a local or remote EDEM process.")

   ("disconnect"
    (edem-is-connected?)
    ti-edem-disconnect
    "Disconnect from the coupled EDEM process.")

   ("synchronize"
    (edem-is-connected?)
    ti-edem-synchronize
    "Synchromnize simulation time on Fluent to the coupled EDEM time.")
   ))

(ti-menu-insert-item! define-menu (make-menu-item "edem-coupling/" #t ti-edem-coupling-menu "Set up the EDEM coupling."))
