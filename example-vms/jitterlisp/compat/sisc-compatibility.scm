;;; SISC -*- Scheme -*- compatibility macros to run JitterLisp code.

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
;;; along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>. */


;;;; Predecessor and successor.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; These are performance-critical and I use them a lot in JitterLisp, so it's
;; better not to introduce a procedure layer.
(define-syntax 1- (syntax-rules () ((_ n) (- n 1))))
(define-syntax 1+ (syntax-rules () ((_ n) (+ n 1))))




;;;; Iteration.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Define a while loop macro.
(define-syntax while
  (syntax-rules ()
    ((_ guard body-forms ...)
     (let loop ()
       (if guard
           (begin
             body-forms ...
             (loop)))))))
