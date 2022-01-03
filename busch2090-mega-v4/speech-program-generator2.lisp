(defun microtronic-speech-program (string name)
  (when (> (length string) 126)
    (error "Won't fit in Microtronic memory"))

  (with-open-file (stream name
                          :direction :output
                          :if-exists :supersede
                          :if-does-not-exist :create)
    (format stream "# Say ~A" string) 
    (dolist (char (nconc (cons #\S (coerce string 'list))
                         (list #\return)))
      (let ((code (char-code char)))
        (multiple-value-bind (a b) 
            (floor code 10) 
          (format stream "~%0~X~X" a a)
          (format stream "~%0~X~X" b b))))))
