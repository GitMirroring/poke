;;; SIOD -*- Scheme -*- compatibility macros to run JitterLisp code.

;;; Copyright (C) 2017 Luca Saiu
;;; Written by Luca Saiu

;;; This file is part of the JitterLisp language implementation, distributed as
;;; an example along with GNU Jitter under the same license.

;;; Jitter is free software: you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation, either version 3 of the License, or
;;; (at your option) any later version.

;;; Jitter is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.

;;; You should have received a copy of the GNU General Public License
;;; along with GNU Jitter.  If not, see <https://www.gnu.org/licenses/>. */


;;;; This is painful.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;; SIOD doesn't seem to have macros at all, or even quasiquoting.




;;;; Predecessor and successor.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (1+ n) (+ n 1))
(define (1- n) (- n 1))




;;;; Missing functions.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (zero? n)
  (eq? n 0))

;; SIOD has only flonums.  The trunc procedure rounds towards minus infinity.
(define (quotient n m)
  (trunc (/ n m)))
(define (remainder n m)
  (- n (* m (quotient n m))))

(define (display x)
  (prin1 x))
(define (newline)
  (print))




;;;; Iteration.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Luckily a while loop is predefined.
