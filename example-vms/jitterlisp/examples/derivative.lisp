;;; JitterLisp (almost -*- Scheme -*- for Emacs): symbolic derivatives.

;;; Copyright (C) 2017, 2018 Luca Saiu
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


;;;; Utility.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (symbolic-square exp)
  `(* ,exp ,exp))




;;;; Arity normalization.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (normalize-arity exp)
  (cond ((symbol? exp)
         exp)
        ((number? exp)
         exp)
        ((not (cons? exp))
         (error `(normalize-arity: invalid exp ,exp)))
        (#t
         (normalize-arity-combination (car exp) (cdr exp)))))

(define (normalize-arity-combination rator rands)
  (let ((normalized-rands (map normalize-arity rands)))
    (cond ((eq? rator '+)
           (normalize-arity-+ normalized-rands))
          ((eq? rator '-)
           (normalize-arity-- normalized-rands))
          ((eq? rator '*)
           (normalize-arity-* normalized-rands))
          ((eq? rator '/)
           (normalize-arity-/ normalized-rands))
          ((eq? rator '**)
           (if (>= (length rands) 2)
               (normalize-arity-** normalized-rands)
               (error `(invalid ** arity: ,@rands))))
          ((eq? rator 'exp)
           (if (= (length rands) 1)
               `(,rator ,(car normalized-rands))
               (error `(invalid exp arity: ,@rands))))
          (#t
           (if (null? (cdr rands))
               `(,rator ,(car normalized-rands))
               (error `(invalid non-unary combination (,rator ,@rands))))))))

(define (normalize-arity-+ normalized-rands)
  (normalize-arity-associative '+ 0 normalized-rands))
(define (normalize-arity-* normalized-rands)
  (normalize-arity-associative '* 1 normalized-rands))
(define (normalize-arity-- normalized-rands)
  (let ((normalized-rands-length (length normalized-rands)))
    (cond ((= normalized-rands-length 1)
           `(negate ,@normalized-rands))
          ((= normalized-rands-length 2)
           (if (eq? (car normalized-rands) 0)
               `(negate ,(cadr normalized-rands))
               (normalize-arity-anti-associative '- 0 '+ normalized-rands)))
          (#t
           (normalize-arity-anti-associative '- 0 '+ normalized-rands)))))
(define (normalize-arity-/ normalized-rands)
  (if (= (length normalized-rands) 1)
      `(/ 1 ,@normalized-rands)
      (normalize-arity-anti-associative '/ 1 '* normalized-rands)))

(define (normalize-arity-associative rator neutral normalized-rands)
  (let ((length (length normalized-rands)))
    (cond ((= length 0)
           neutral)
          ((= length 1)
           (car normalized-rands))
          ((= length 2)
           `(,rator ,@normalized-rands))
          (#t
           `(,rator ,(car normalized-rands)
                    (,rator ,(cadr normalized-rands)
                            ,(normalize-arity-associative rator
                                                          neutral
                                                          (cddr normalized-rands))))))))

(define (normalize-arity-anti-associative rator right-neutral opposite
                                          normalized-rands)
  (let ((length (length normalized-rands)))
    (cond ((= length 0)
           (error `(nullary ,rator)))
          ((= length 1)
           `(,rator ,@normalized-rands))
          ((= length 2)
           `(,rator ,@normalized-rands))
          (#t
           `(,rator ,(car normalized-rands)
                    ,(normalize-arity `(,opposite ,@(cdr normalized-rands))))))))

;; FIXME: generalize to generic right-associative operators?
(define (normalize-arity-** normalized-rands)
  (let ((length (length normalized-rands)))
    (if (null? (cddr normalized-rands))
        `(** ,@normalized-rands)
        `(** ,(car normalized-rands)
             ,(normalize-arity-** (cdr normalized-rands))))))




;;;; Normalization.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (normalize exp)
  ;; Right now we only normalize with respect to arity.
  (normalize-arity exp))




;;;; Derivative of a normalized expression.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; This assumes that exp is already normalized.
(define (derivative-recursive exp x)
  (cond ((number? exp)
         0)
        ((symbol? exp)
         (if (eq? x exp)
             1
             0))
        ((or (eq? (car exp) '+)
             (eq? (car exp) '-))
         `(,(car exp) ,(derivative-recursive (cadr exp) x)
                      ,(derivative-recursive (caddr exp) x)))
        ((eq? (car exp) 'negate)
         `(negate ,(derivative-recursive (cadr exp) x)))
        ((eq? (car exp) '*)
         `(+ (* ,(derivative-recursive (cadr exp) x)
                ,(caddr exp))
             (* ,(cadr exp)
                ,(derivative-recursive (caddr exp) x))))
        ((eq? (car exp) '/)
         `(/ (- (* ,(derivative-recursive (cadr exp) x)
                   ,(caddr exp))
                (* ,(cadr exp)
                   ,(derivative-recursive (caddr exp) x)))
             (** ,(caddr exp) 2)))
        ((eq? (car exp) 'sin)
         `(* (cos ,(cadr exp))
             ,(derivative-recursive (cadr exp) x)))
        ((eq? (car exp) 'cos)
         `(* (- 0 (sin ,(cadr exp)))
             ,(derivative-recursive (cadr exp) x)))
        ((eq? (car exp) 'tan)
         `(/ ,(derivative-recursive (cadr exp) x)
             (** (cos ,(cadr exp)) 2)))
        ((eq? (car exp) 'exp)
         `(* (exp ,(cadr exp))
             ,(derivative-recursive (cadr exp) x)))
        ((eq? (car exp) 'ln)
         `(/ ,(derivative-recursive (cadr exp) x)
             ,(cadr exp)))
        ((eq? (car exp) '**)
         (let* ((f (cadr exp))
                (g (caddr exp))
                (df/dx (derivative-recursive f x))
                (dg/dx (derivative-recursive g x)))
           `(* (** ,f ,g)
               (+ (* ,df/dx (/ ,g ,f))
                  (* ,dg/dx (ln ,f))))))
        (#t
         (error 'unimplemented))))




;;;; Unnormalization.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; This is a syntactically recursive process, by its nature extremely crude.
;; Producing a simple expression from an arbitrarily obfuscated one is a nice
;; optimization problem, out of scope for this example.

(define (associative-rator? rator)
  (cond ((eq? rator '+)
         #t)
        ((eq? rator '*)
         #t)
        (#t
         #f)))

(define (rator->neutral rator)
  (cond ((eq? rator '+)
         0)
        ((eq? rator '*)
         1)
        (#t
         (error `(rator->neutral: unknown rator ,rator)))))

(define (has-absorbing? rator)
  (cond ((eq? rator '*)
         #t)
        (#t
         #f)))

(define (rator->absorbing rator)
  (cond ((eq? rator '*)
         0)
        (#t
         (error `(rator->absorbing: no (l-r) absorbing element for ,rator)))))

(define (commutative-rator? rator)
  (cond ((eq? rator '+)
         #t)
        ((eq? rator '*)
         #t)
        (#t
         #f)))

(define (unnormalize-wrt-associativity exp)
  (cond ((number? exp)
         exp)
        ((symbol? exp)
         exp)
        ((not (cons? exp))
         (error `(invalid expression to unnormalize-wrt-associativity: ,exp)))
        ((associative-rator? (car exp))
         (unnormalize-associative (car exp) (cdr exp)))
        (#t
         `(,(car exp) ,@(map unnormalize-wrt-associativity (cdr exp))))))

(define (unnormalize-associative rator rands)
  (let* ((unnomralized-rands (map unnormalize rands))
         (raised-rands (raise rator unnomralized-rands))
         (raised-rand-no (length raised-rands)))
    (cond ((= raised-rand-no 0)
           (rator->neutral rator))
          ((= raised-rand-no 1)
           (car raised-rands))
          (#t
           `(,rator ,@raised-rands)))))

;; Return a list of rands equivalent to the given one, with depth-1 applications
;; of the same given rator raised to the top level.
;; For example (raise '+ '(1 (+ 2) a (+) (+ b c d) (* 3 e) f)) will return
;; (1 2 a b c d (* 3 e) f) .
(define (raise rator rands)
  (cond ((null? rands)
         '())
        ((not (cons? (car rands)))
         (cons (car rands)
               (raise rator (cdr rands))))
        ((eq? (caar rands) rator)
         (append (cdar rands)
                 (raise rator (cdr rands))))
        (#t
         (cons (car rands)
               (raise rator (cdr rands))))))

(define (unnormalize-wrt-commutativity exp)
  (cond ((number? exp)
         exp)
        ((symbol? exp)
         exp)
        ((not (cons? exp))
         (error `(invalid expression to unnormalize-wrt-associativity: ,exp)))
        ((commutative-rator? (car exp))
         (unnormalize-commutative (car exp) (cdr exp)))
        (#t
         `(,(car exp) ,@(map unnormalize-wrt-commutativity (cdr exp))))))

(define (rator->procedure rator)
  (cond ((eq? rator '+)
         (lambda (x y) (+ x y)))
        ((eq? rator '*)
         (lambda (x y) (* x y)))
        (#t
         (error `(rator->procedure: invalid rator ,rator)))))

(define (join-numbers-helper rator-procedure numeric-rands acc)
  (if (null? numeric-rands)
      acc
      (join-numbers-helper rator-procedure
                           (cdr numeric-rands)
                           (rator-procedure (car numeric-rands)
                                            acc))))
(define (join-numbers rator numeric-rands)
  (join-numbers-helper (rator->procedure rator)
                       numeric-rands
                       (rator->neutral rator)))

(define (unnormalize-commutative rator rands)
  (let* ((numbers (filter number? rands))
         (non-numbers (filter (lambda (x) (not (number? x))) rands))
         (joined-number (join-numbers rator numbers))
         (neutral (rator->neutral rator)))
    (cond ((null? non-numbers)
           joined-number)
          ((= joined-number neutral)
           (if (null? (cdr non-numbers))
               (car non-numbers)
               `(,rator ,@non-numbers)))
          ((has-absorbing? rator)
           (let ((absorbing (rator->absorbing rator)))
             (if (= joined-number absorbing)
                 absorbing
                 `(,rator ,joined-number ,@non-numbers))))
          (#t
           `(,rator ,joined-number ,@non-numbers)))))

(define (without-negate exp)
  (cond ((number? exp)
         exp)
        ((symbol? exp)
         exp)
        ((eq? (car exp) 'negate)
         (let ((cadr-simplified (without-negate (cadr exp))))
           (if (number? cadr-simplified)
               (- 0 cadr-simplified)
               `(- ,cadr-simplified))))
        (#t
         `(,(car exp) ,@(map without-negate (cdr exp))))))

(define (unnormalize exp)
  (let* ((exp-1 (unnormalize-wrt-associativity exp))
         (exp-2 (unnormalize-wrt-commutativity exp-1))
         (exp-3 (without-negate exp-2)))
    exp-3))




;;;; Simplification.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; An expression is made simpler, usually, by first normalizing it (which
;; introduces negate operators) and then unnormalizing it.
(define (simplify exp)
  (unnormalize (normalize exp)))




;;;; Derivative of an arbitrary expression.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (derivative exp x)
  (derivative-recursive (normalize (simplify exp)) x))




;;;; A simple interactive Read-Derive-Print Loop.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (derivative-repl)
  (display '(Please enter an expression to be derived with respect to x))
  (newline)
  (let ((exp (read)))
    (if (not (eof? exp))
        (let* ((exp-simplified (simplify exp))
               (exp-simplified-normalized (normalize exp-simplified)))
          (display `(the derivative of ,exp is:))
          (newline)
          (display `((which is to say the derivative of ,exp-simplified-normalized is:)))
          (newline)
          (let* ((dexp/dx (derivative exp 'x))
                 (dexp/dx-simplified (simplify dexp/dx)))
            ;; (display dexp/dx)
            ;; (newline)
            ;; (display `(which is to say:))
            ;; (newline)
            (display dexp/dx-simplified)
            (newline)
            (newline)
            (derivative-repl))))))
