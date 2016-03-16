(in-package :user)

(defvar *gdcreate* "/home/alb/bin/gdcreate")
(defvar *gd* t "Stream where to send gd commands.")

(defun send (&rest strings)
  (dolist (s strings)
    (princ s *gd*)
    (terpri *gd*))
  (terpri *gd*))

(defun receive (&optional code)
  (let ((line (read-line *gd*)))
    (when code
      (unless (and (> (length line) 1)
		   (string= (subseq line 0 2) code))
	(error "Error reading response from GD processor: expected `~a', got `~a'."
	       code line)))))

;;; Library

(defun create-image (x y)
  (send "// Create image" "CR" x y))

(defun save-image (filename)
  (send "// Save image" "SA" filename))

(defun allocate-color (red green blue)
  (send "// Allocate color" "CA" red green blue))

(defun draw-pixel (x y c)
  (send "// Draw pixel" "PI" x y c))

(defun draw-line (x1 y1 x2 y2 c)
  (send "// Draw line" "LI" x1 y1 x2 y2 c))

(defun draw-vector (x y dx dy c)
  (send "// Draw line" "LI" x y (+ x dx) (+ y dy) c))

(defun draw-rectangle (x1 y1 x2 y2 c)
  (send "// Draw rectangle" "RE" x1 y1 x2 y2 c))

(defun draw-filled-rectangle (x1 y1 x2 y2 c)
  (send "// Draw filled rectangle" "RF" x1 y1 x2 y2 c))

(defun draw-string (font x y c string)
  (send "// Draw string" "ST" font x y c string))

(defun draw-string-up (font x y c string)
  (send "// Draw string up" "SU" font x y c string))

(defun image-fill (x y c)
  (send "// Fill" "FI" x y c))

(defun draw-circle (x y r c)
  (send "// Circle" "CI" x y r c))

(defun draw-filled-ellipse (x y w h c)
  (send "// FIlled ellipse" "EF" x y w h c))

(defun set-viewport (bx1 by1 bx2 by2 vx1 vy1 vx2 vy2)
  (send "// Set viewport" "VI" bx1 by1 bx2 by2 vx1 vy1 vx2 vy2))

(defun clear-viewport ()
  (send "// Clear viewport" "VO"))

(defun zz ()
  (send "// Quit" "ZZ"))

;;; Top-level

(defmacro with-gd-file ((filename &rest open-options) &body forms)
  `(with-open-file (*gd* ,filename :direction :output
			           :if-exists :supersede
				   :if-does-not-exist :create
				   ,@open-options)
     ,@forms))

(defmacro with-gd-proc (&body forms)
  `(let ((*gd* (ext:run-program *gdcreate* :input :stream :wait t)))
     (unwind-protect
	  (progn ,@forms (zz))
       (when (streamp *gd*)
	 (close *gd*)))))

(defmacro with-viewport (coords &body forms)
  `(progn
     (set-viewport ,@coords)
     ,@forms
     (clear-viewport)))

;;; Examples

(defun test1 ()
  (with-gd-proc
      (create-image 600 400)
    (allocate-color 255 255 255)	; 0
    (allocate-color 0 0 0)		; 1
    (allocate-color 255 0 0)		; 2
    (draw-rectangle 20 20 580 380 1)
    (draw-line 20 20 580 380 2)
    (draw-rectangle 60 60 540 340 1)
    (image-fill 65 65 2)
    (save-image "test.png")))

(defun test-viewport ()
  (with-gd-proc
    (create-image 600 400)
    (allocate-color 255 255 255)	; 0
    (allocate-color 0 0 0)		; 1
    (allocate-color 255 0 0)            ; 2
    (allocate-color 0 0 255)            ; 3
    (allocate-color 200 200 200)	; 4
    (draw-rectangle 50 50 550 350 1)
    (draw-rectangle 49 49 551 351 1)
    (draw-circle 300 200 100 4)
    (image-fill 300 200 4)
    (with-viewport (50 50 550 350 0 0 5 5)
      (draw-circle 2.5 2.5 0.5 1)
      (loop
	 for i from 0.5 to 4.5 by 0.5
	 do (draw-line i 0 i 5 2)
	   (draw-line 0 i 5 i 3)))
    (draw-string 1 60 35 1 "Viewport demo!")
    (save-image "test.png")))
