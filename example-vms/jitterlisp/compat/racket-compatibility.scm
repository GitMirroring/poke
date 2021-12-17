;;; Racket -*- Scheme -*- compatibility macros to run JitterLisp code.

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


;;;; Mutable pairs.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Racket's pairs are immutable.  Use the mutable-pair library.
(require rnrs/mutable-pairs-6)

;;; Redefine fundamental pairs primitives as macros using mutable pairs
;;; procedures.
(define-syntax pair? (syntax-rules () ((_ c) (mpair? c))))
(define-syntax cons (syntax-rules () ((_ a d) (mcons a d))))
(define-syntax car (syntax-rules () ((_ c) (mcar c))))
(define-syntax cdr (syntax-rules () ((_ c) (mcdr c))))
(define-syntax set-car! (syntax-rules () ((_ c a) (set-mcar! c a))))
(define-syntax set-cdr! (syntax-rules () ((_ c d) (set-mcdr! c d))))

;;; There are no composed pair selectors such as mcadr in the library.
;;; Define them as macros.
(define-syntax caar (syntax-rules () ((_ c) (car (car c)))))
(define-syntax cadr (syntax-rules () ((_ c) (car (cdr c)))))
(define-syntax cdar (syntax-rules () ((_ c) (cdr (car c)))))
(define-syntax cddr (syntax-rules () ((_ c) (cdr (cdr c)))))
(define-syntax caaar (syntax-rules () ((_ c) (caar (car c)))))
(define-syntax caadr (syntax-rules () ((_ c) (caar (cdr c)))))
(define-syntax cadar (syntax-rules () ((_ c) (cadr (car c)))))
(define-syntax caddr (syntax-rules () ((_ c) (cadr (cdr c)))))
(define-syntax cdaar (syntax-rules () ((_ c) (cdar (car c)))))
(define-syntax cdadr (syntax-rules () ((_ c) (cdar (cdr c)))))
(define-syntax cddar (syntax-rules () ((_ c) (cddr (car c)))))
(define-syntax cdddr (syntax-rules () ((_ c) (cddr (cdr c)))))
(define-syntax caaaar (syntax-rules () ((_ c) (caaar (car c)))))
(define-syntax caaadr (syntax-rules () ((_ c) (caaar (cdr c)))))
(define-syntax caadar (syntax-rules () ((_ c) (caadr (car c)))))
(define-syntax caaddr (syntax-rules () ((_ c) (caadr (cdr c)))))
(define-syntax cadaar (syntax-rules () ((_ c) (cadar (car c)))))
(define-syntax cadadr (syntax-rules () ((_ c) (cadar (cdr c)))))
(define-syntax caddar (syntax-rules () ((_ c) (caddr (car c)))))
(define-syntax cadddr (syntax-rules () ((_ c) (caddr (cdr c)))))
(define-syntax cdaaar (syntax-rules () ((_ c) (cdaar (car c)))))
(define-syntax cdaadr (syntax-rules () ((_ c) (cdaar (cdr c)))))
(define-syntax cdadar (syntax-rules () ((_ c) (cdadr (car c)))))
(define-syntax cdaddr (syntax-rules () ((_ c) (cdadr (cdr c)))))
(define-syntax cddaar (syntax-rules () ((_ c) (cddar (car c)))))
(define-syntax cddadr (syntax-rules () ((_ c) (cddar (cdr c)))))
(define-syntax cdddar (syntax-rules () ((_ c) (cdddr (car c)))))
(define-syntax cddddr (syntax-rules () ((_ c) (cdddr (cdr c)))))

;; Same thing with procedures.  Tentatively disabled.
;; (define (caar c) (car (car c)))
;; (define (cadr c) (car (cdr c)))
;; (define (cdar c) (cdr (car c)))
;; (define (cddr c) (cdr (cdr c)))
;; (define (caaar c) (caar (car c)))
;; (define (caadr c) (caar (cdr c)))
;; (define (cadar c) (cadr (car c)))
;; (define (caddr c) (cadr (cdr c)))
;; (define (cdaar c) (cdar (car c)))
;; (define (cdadr c) (cdar (cdr c)))
;; (define (cddar c) (cddr (car c)))
;; (define (cdddr c) (cddr (cdr c)))
;; (define (caaaar c) (caaar (car c)))
;; (define (caaadr c) (caaar (cdr c)))
;; (define (caadar c) (caadr (car c)))
;; (define (caaddr c) (caadr (cdr c)))
;; (define (cadaar c) (cadar (car c)))
;; (define (cadadr c) (cadar (cdr c)))
;; (define (caddar c) (caddr (car c)))
;; (define (cadddr c) (caddr (cdr c)))
;; (define (cdaaar c) (cdaar (car c)))
;; (define (cdaadr c) (cdaar (cdr c)))
;; (define (cdadar c) (cdadr (car c)))
;; (define (cdaddr c) (cdadr (cdr c)))
;; (define (cddaar c) (cddar (car c)))
;; (define (cddadr c) (cddar (cdr c)))
;; (define (cdddar c) (cdddr (car c)))
;; (define (cddddr c) (cdddr (cdr c)))




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
             (loop))
           'done))))) ;; Racket wants a two-branch if .
