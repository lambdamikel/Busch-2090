(defun microtronic-speech-program (string name)
  (when (> (length string) 58)
    (error "Won't fit in Microtronic memory"))

  (with-open-file (stream name
                          :direction :output
                          :if-exists :supersede
                          :if-does-not-exist :create)
    (format stream "# Say ~A" string) 
    (format stream "~%BC0")
    (dolist (char (coerce string 'list))
      (let ((code (char-code char)))
        (multiple-value-bind (a b) 
            (floor code 10) 
          (format stream "~%1~X0" a)
          (format stream "~%1~X1" b)
          (format stream "~%BE0" b))))
    (format stream "~%CF0")
    (format stream "~%# send start 
# send N = 78 -> 89 
# \cr = 24 -> 13 
# send S = 83 -> 94 

@ C0 

F08
FE2 
180 
191 
FE0
FE2
FE1
FE2
120  
141 
FE0
FE2
FE1 
FE2
190
141 
FE0
FE2
FE1
FE2 
F07

# send char 
# ascii xy = x-1 y-1 

@ E0

510
511
F20
FE0
FE2
FE1
FE2
F02 
F07

# send end 
# cr = 24 = 13 

@ F0

120  
141 
FE0
FE2
FE1 
FE2")))


