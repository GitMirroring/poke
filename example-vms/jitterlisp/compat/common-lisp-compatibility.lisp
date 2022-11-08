;;; -*- Common-Lisp -*- compatibility macros to run JitterLisp code.

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


;;;; Introduction.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; This is a very simple and limited wrapper providing JitterLisp-style
;;; (which tends to be Scheme-style) syntax, so that JitterLisp code can
;;; be run on top of a Common Lisp system.

;;; The compatibility wrapper is defined using macros even when Common Lisp
;;; functions would have worked.  The idea, of course, is comparing the
;;; performance of production Common Lisp systems with JitterLisp's.




;;;; Scratch.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; This lets me redefine some fundamental functions in CLISP, but is probably a
;;; bad idea.  It doesn't work on SBCL, even if SBCL has a similar mechanism
;; (setq CUSTOM:*SUPPRESS-CHECK-REDEFINITION* t)
;;(setf (ext:package-lock 'common-lisp) nil)

;; (defpackage #:foo (:use :cl) (:export #:bar #:quux))
;; (in-package #:foo)
;; (setq *package* #:foo)




;;;; Scheme-style define.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defmacro define (defined-thing &rest body)
  (if (symbolp defined-thing)
      `(setq ,defined-thing
             (progn ,@body))
    `(defun ,(car defined-thing) ,(cdr defined-thing)
       ,@body)))




;;;; While loop.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defmacro while (condition &rest body-forms)
  `(do ()
       ((not ,condition))
     ,@body-forms))




;;;; Scheme-style assignment (with JitterLisp-style multiple forms).
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defmacro set! (variable &rest body-forms)
  `(setq ,variable
         (progn
           ,@body-forms)))




;;;; Scheme-style expression sequence.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defmacro begin (&rest body-forms)
  `(progn
     ,@body-forms))




;;;; Scheme-style cons setters.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defmacro set-car! (cons new-car)
  `(rplaca ,cons ,new-car))

(defmacro set-cdr! (cons new-car)
  `(rplacd ,cons ,new-car))




;;;; Scheme-style booleans.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Define #t and #f as trivial reader macros, not actually reading
;;; anything after the dispatch character #\f or #\t .
(set-dispatch-macro-character #\# #\f
  #'(lambda(s c n)
      nil))
(set-dispatch-macro-character #\# #\t
  #'(lambda(s c n)
      t))




;;;; Scheme-style predicates.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defmacro null? (thing)
  `(null ,thing))
(defmacro zero? (thing)
  `(zerop ,thing))
(defmacro quotient (a b)
  `(floor ,a ,b))
(defmacro remainder (a b)
  `(rem ,a ,b))




;;;; I/O.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defmacro display (object)
  `(format t "~a" ,object))
(defmacro newline ()
  `(format t "~%"))
