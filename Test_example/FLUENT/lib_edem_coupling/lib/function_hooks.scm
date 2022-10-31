
(define (add-init-udf udf-name)
  (let ((func-list (rpgetvar 'udf/init-fcns)))
    (if (not (member udf-name func-list))
        (rpsetvar 'udf/init-fcns (list-add func-list udf-name)))))

(define (remove-init-udf udf-name)
  (let ((func-list (rpgetvar 'udf/init-fcns)))
    (if (member udf-name func-list)
        (rpsetvar 'udf/init-fcns (list-remove func-list udf-name)))))

(define (add-adjust-udf udf-name)
  (let ((func-list (rpgetvar 'udf/adjust-fcns)))
    (if (not (member udf-name func-list))
        (rpsetvar 'udf/adjust-fcns (list-add func-list udf-name)))))

(define (remove-adjust-udf udf-name)
  (let ((func-list (rpgetvar 'udf/adjust-fcns)))
    (if (member udf-name func-list)
        (rpsetvar 'udf/adjust-fcns (list-remove func-list udf-name)))))

(define (add-execute-at-end-udf udf-name)
  (let ((func-list (rpgetvar 'udf/execute-at-end-fcns)))
    (if (not (member udf-name func-list))
        (begin
          (rpsetvar 'udf/execute-at-end-fcns (list-add func-list udf-name))
          (register-monitor-execute-at-end-if-needed)
          ))))

(define (remove-execute-at-end-udf udf-name)
  (let ((func-list (rpgetvar 'udf/execute-at-end-fcns)))
    (if (member udf-name func-list)
        (begin
          (rpsetvar 'udf/execute-at-end-fcns (list-remove func-list udf-name))
          (register-monitor-execute-at-end-if-needed)
          ))))

(define (add-read-case-udf udf-name)
  (let ((func-list (rpgetvar 'udf/read-case-fcns)))
    (if (not (member udf-name func-list))
        (rpsetvar 'udf/read-case-fcns (list-add func-list udf-name)))))

(define (remove-read-case-udf udf-name)
  (let ((func-list (rpgetvar 'udf/read-case-fcns)))
    (if (member udf-name func-list)
        (rpsetvar 'udf/read-case-fcns (list-remove func-list udf-name)))))

(define (add-read-data-udf udf-name)
  (let ((func-list (rpgetvar 'udf/read-data-fcns)))
    (if (not (member udf-name func-list))
        (rpsetvar 'udf/read-data-fcns (list-add func-list udf-name)))))

(define (remove-read-data-udf udf-name)
  (let ((func-list (rpgetvar 'udf/read-data-fcns)))
    (if (member udf-name func-list)
        (rpsetvar 'udf/read-data-fcns (list-remove func-list udf-name)))))

(define (add-write-case-udf udf-name)
  (let ((func-list (rpgetvar 'udf/write-case-fcns)))
    (if (not (member udf-name func-list))
        (rpsetvar 'udf/write-case-fcns (list-add func-list udf-name)))))

(define (remove-write-case-udf udf-name)
  (let ((func-list (rpgetvar 'udf/write-case-fcns)))
    (if (member udf-name func-list)
        (rpsetvar 'udf/write-case-fcns (list-remove func-list udf-name)))))

(define (add-write-data-udf udf-name)
  (let ((func-list (rpgetvar 'udf/write-data-fcns)))
    (if (not (member udf-name func-list))
        (rpsetvar 'udf/write-data-fcns (list-add func-list udf-name)))))

(define (remove-write-data-udf udf-name)
  (let ((func-list (rpgetvar 'udf/write-data-fcns)))
    (if (member udf-name func-list)
        (rpsetvar 'udf/write-data-fcns (list-remove func-list udf-name)))))

(define (add-execute-at-exit-udf udf-name)
  (let ((func-list (rpgetvar 'udf/execute-at-exit-fcns)))
    (if (not (member udf-name func-list))
        (rpsetvar 'udf/execute-at-exit-fcns (list-add func-list udf-name)))))

(define (remove-execute-at-exit-udf udf-name)
  (let ((func-list (rpgetvar 'udf/execute-at-exit-fcns)))
    (if (member udf-name func-list)
        (rpsetvar 'udf/execute-at-exit-fcns (list-remove func-list udf-name)))))

;; Execute Commands

(define (add-execute-command command-name command-string freq timestep? exec-once-at? active?)
  (let ((cmd-list (or (remove-execute-command command-name) (rpgetvar 'monitor/commands))) ;; Remove current entry if there already and return new list
        (command-entry (list (if (symbol? command-name) command-name (string->symbol command-name)) freq timestep? command-string exec-once-at? active?)))
    (let ((new-cmd-list (list-add cmd-list command-entry)))
      (rpsetvar 'monitor/commands new-cmd-list)
      (register-monitor-commands-if-needed)
      (cx-changed 'execute-command-list)
      new-cmd-list)))

(define (remove-execute-command . command-name)
  (let ((cmd-list (rpgetvar 'monitor/commands)))
    (if (not (null? cmd-list))
        (let* ((command-symbol (if (pair? command-name) (let ((cn (car command-name))) (if (symbol? cn) cn (string->symbol cn))) (caar (reverse cmd-list))))
               (command-entry (assq command-symbol cmd-list)))
          (if command-entry
              (let ((new-cmd-list (list-remove cmd-list command-entry)))
                (rpsetvar 'monitor/commands new-cmd-list)
                (register-monitor-commands-if-needed)
                (cx-changed 'execute-command-list)
                new-cmd-list))))))

(define (add-default-execute-command)
  (let* ((cmd-list (rpgetvar 'monitor/commands))
        (command-entry (list (string->symbol (format #f "command-~a" (+ (length cmd-list) 1))) 1 #f "" #f #f)))
    (let ((new-cmd-list (list-add cmd-list command-entry)))
      (rpsetvar 'monitor/commands new-cmd-list)
      (register-monitor-commands-if-needed)
      (cx-changed 'execute-command-list)
      new-cmd-list)))

