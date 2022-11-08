;;; JitterLisp (let's say -*- Scheme -*- for the purposes of Emacs) -- library.

;;; Copyright (C) 2017, 2018, 2019, 2020 Luca Saiu
;;; Updated in 2021 by Luca Saiu
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




;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; Force the library to be run at most once.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;; Fail if the library has been loaded already.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(if (defined? 'jitterlisp-library-loaded)
    (error '(you are trying to load the library more than once))
    #t)




;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; Type checking.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;; anything?.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; A type predicate always returning #t.
(define-constant (anything? x)
  #t)




;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; Arithmetic and number library.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;; Parity: even? and odd?.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (even? n)
  (zero? (remainder n 2)))

(define-constant (odd? n)
  (not (zero? (remainder n 2))))




;;;; number?.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (number? n)
  (fixnum? n)) ;; There is only one number type right now.




;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; Conses.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;; Composed cons selectors.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; I define these early because some composed selectors are used within the
;;; implementation of high-level macros.

;; Length 2.
(define-constant (caar x) (car (car x)))
(define-constant (cadr x) (car (cdr x)))
(define-constant (cdar x) (cdr (car x)))
(define-constant (cddr x) (cdr (cdr x)))

;; Length 3.
(define-constant (caaar x) (car (caar x)))
(define-constant (caadr x) (car (cadr x)))
(define-constant (cadar x) (car (cdar x)))
(define-constant (caddr x) (car (cddr x)))
(define-constant (cdaar x) (cdr (caar x)))
(define-constant (cdadr x) (cdr (cadr x)))
(define-constant (cddar x) (cdr (cdar x)))
(define-constant (cdddr x) (cdr (cddr x)))

;; Length 4.
(define-constant (caaaar x) (car (caaar x)))
(define-constant (caaadr x) (car (caadr x)))
(define-constant (caadar x) (car (cadar x)))
(define-constant (caaddr x) (car (caddr x)))
(define-constant (cadaar x) (car (cdaar x)))
(define-constant (cadadr x) (car (cdadr x)))
(define-constant (caddar x) (car (cddar x)))
(define-constant (cadddr x) (car (cdddr x)))
(define-constant (cdaaar x) (cdr (caaar x)))
(define-constant (cdaadr x) (cdr (caadr x)))
(define-constant (cdadar x) (cdr (cadar x)))
(define-constant (cdaddr x) (cdr (caddr x)))
(define-constant (cddaar x) (cdr (cdaar x)))
(define-constant (cddadr x) (cdr (cdadr x)))
(define-constant (cdddar x) (cdr (cddar x)))
(define-constant (cddddr x) (cdr (cdddr x)))




;;;; Composed cons updaters.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Length 2.
(define-constant (set-caar! c x) (set-car! (car c) x))
(define-constant (set-cadr! c x) (set-car! (cdr c) x))
(define-constant (set-cdar! c x) (set-cdr! (car c) x))
(define-constant (set-cddr! c x) (set-cdr! (cdr c) x))

;; Length 3.
(define-constant (set-caaar! c x) (set-car! (caar c) x))
(define-constant (set-caadr! c x) (set-car! (cadr c) x))
(define-constant (set-cadar! c x) (set-car! (cdar c) x))
(define-constant (set-caddr! c x) (set-car! (cddr c) x))
(define-constant (set-cdaar! c x) (set-cdr! (caar c) x))
(define-constant (set-cdadr! c x) (set-cdr! (cadr c) x))
(define-constant (set-cddar! c x) (set-cdr! (cdar c) x))
(define-constant (set-cdddr! c x) (set-cdr! (cddr c) x))

;; Length 4.
(define-constant (set-caaaar! c x) (set-car! (caaar c) x))
(define-constant (set-caaadr! c x) (set-car! (caadr c) x))
(define-constant (set-caadar! c x) (set-car! (cadar c) x))
(define-constant (set-caaddr! c x) (set-car! (caddr c) x))
(define-constant (set-cadaar! c x) (set-car! (cdaar c) x))
(define-constant (set-cadadr! c x) (set-car! (cdadr c) x))
(define-constant (set-caddar! c x) (set-car! (cddar c) x))
(define-constant (set-cadddr! c x) (set-car! (cdddr c) x))
(define-constant (set-cdaaar! c x) (set-cdr! (caaar c) x))
(define-constant (set-cdaadr! c x) (set-cdr! (caadr c) x))
(define-constant (set-cdadar! c x) (set-cdr! (cadar c) x))
(define-constant (set-cdaddr! c x) (set-cdr! (caddr c) x))
(define-constant (set-cddaar! c x) (set-cdr! (cdaar c) x))
(define-constant (set-cddadr! c x) (set-cdr! (cdadr c) x))
(define-constant (set-cdddar! c x) (set-cdr! (cddar c) x))
(define-constant (set-cddddr! c x) (set-cdr! (cdddr c) x))




;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; Quasiquoting.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;; quasiquote-procedure.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Define temporary list functions of a few different arities.  It is
;;; not worth the trouble to define list as a variadic macro without
;;; high-level macros or quasiquoting.
(define-constant (list0)         ())
(define-constant (list1 x)       (cons x ()))
(define-constant (list2 x y)     (cons x (cons y ())))
(define-constant (list3 x y z)   (cons x (cons y (cons z ()))))
(define-constant (list4 x y z t) (cons x (cons y (cons z (cons t ())))))

(define-constant (qq-append xs ys)
  (if (list? xs)
      (append-procedure xs ys)
      (error '(unquote-splicing argument evaluates to non-list))))

(define-constant (qq-atom x)
  ;; `',x
  (list2 'quote
         x))

(define-constant (qq-recursive x depth)
  (if (cons? x)
      (qq-recursive-cons (car x) (cdr x) depth)
      (qq-atom x)))

;;; ~/r6rs.pdf, §11.17 "Quasiquotation".

(define-constant (qq-recursive-cons x-car x-cdr depth)
  (cond ((eq? x-car 'quasiquote)
         ;;`(list 'quasiquote ,(qq-recursive (car x-cdr) (1+ depth)))
         (list3 'list2
                ''quasiquote
                (qq-recursive (car x-cdr) (1+ depth))))
        ((eq? x-car 'unquote)
         ;; FIXME: decide what to do with (cdr x-cdr).
         (if (zero? depth)
             (car x-cdr)
             ;; `(cons 'unquote ,(qq-recursive x-cdr (1- depth)))
             (list3 'cons
                    ''unquote
                    (qq-recursive x-cdr (1- depth)))))
        ((eq? x-car 'unquote-splicing)
         ;; FIXME: decide what to do with (cdr x-cdr).  This case is
         ;; probably different from the one above.  Look at R7RS as well.
         (if (zero? depth)
             (begin
               ;; There are several possibilities in this case.
               ;; FIXME: consider them, including returning (car x-cdr).
               (error '(invalid context for unquote-splicing)))
             (begin
               ;; `(cons 'unquote-splicing ,(qq-recursive x-cdr (1- depth)))
               (list3 'cons
                      ''unquote-splicing
                      (qq-recursive x-cdr (1- depth))))))
        (else
         ;;`(append-procedure ,(qq-recursive-as-car x-car depth)
         ;;                   ,(qq-recursive x-cdr depth))
         (list3 'append-procedure ;; no qq-append here: qq-recursive-as-car returns a list.
                (qq-recursive-as-car x-car depth)
                (qq-recursive x-cdr depth)))))

;;; Return an s-expression evaluating to a singleton list containing the
;;; given object.
(define-constant (qq-sigleton-expression expression)
  (list2 'list1
         expression))

;;; Return an s-expression evaluating to something equivalent to a variadic
;;; call to the list function of the given arguments.
(define-constant (qq-variadic-list-expression args)
  (if (null? args)
      '()
      ;;`(cons ,(car args) ,(qq-variadic-list-expression (cdr args)))))
      (list3 'cons
             (car args)
             (qq-variadic-list-expression (cdr args)))))

;;; Same as qq-variadic-list-expression but for a variadic append.
(define-constant (qq-variadic-append-expression args)
  (if (null? args)
      '()
      ;;`(append ,(car args) ,(qq-variadic-append-expression (cdr args)))))
      (list3 'qq-append
             (car args)
             (qq-variadic-append-expression (cdr args)))))

;;; The car of a quasiquoted cons expands to a *list*, to be combined
;;; to the expansion of the cdr by appending, not consing.  This allows
;;; for simpler handling of unquote-splicing; when not splicing we just
;;; generate a singleton list.
(define-constant (qq-recursive-as-car x depth)
  (if (cons? x)
      (qq-recursive-cons-as-car (car x) (cdr x) depth)
      ;; `(list ,(qq-atom x))
      (qq-sigleton-expression (qq-atom x))))

;;; Expand a cons which is the car of a bigger quasiquoted s-expression;
;;; therefore, expand to a list as qq-recursive-as-car does.
(define-constant (qq-recursive-cons-as-car x-car x-cdr depth)
  (cond ((eq? x-car 'quasiquote)
         ;; (qq-sigleton-expression `(cons 'quasiquote ,(qq-recursive x-cdr (1+ depth))))
         (qq-sigleton-expression (list3 'cons
                                        ''quasiquote
                                        (qq-recursive x-cdr (1+ depth)))))
        ((eq? x-car 'unquote)
         (if (zero? depth)
             (qq-variadic-list-expression x-cdr)
             ;; `(list (cons 'unquote ,(qq-recursive x-cdr (1- depth))))
             (list2 'list1
                    (list3 'cons
                           ''unquote
                           (qq-recursive x-cdr (1- depth))))))
        ((eq? x-car 'unquote-splicing)
         (if (zero? depth)
             (qq-variadic-append-expression x-cdr)
             ;;`(list (cons 'unquote-splicing ,(qq-recursive x-cdr (1- depth))))
             (list2 'list1
                    (list3 'cons
                           ''unquote-splicing
                           (qq-recursive x-cdr (1- depth))))))
        (else
         ;;`(list (append-procedure ,(qq-recursive-as-car x-car depth)
         ;;                         ,(qq-recursive x-cdr depth))))
         (list2 'list1
                (list3 'append-procedure
                       (qq-recursive-as-car x-car depth)
                       (qq-recursive x-cdr depth))))))

;;; Quasiquoting as a procedure.
(define-constant (quasiquote-procedure x)
  (qq-recursive x 0))




;;;; quasiquote.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Now we can finally define quasiquote as a (low-level) macro.  It is trivial:
;;; low-level-macro-args is bound to all of the arguments of quasiquote , and we
;;; want to fail if those arguments are not a singleton list.  If the arguments
;;; are a singleton list we just call quasiquote-procedure on their car.
(define-constant quasiquote
  (low-level-macro
    (cond ((null? low-level-macro-args)
           (error '(quasiquote with zero arguments)))
          ((non-cons? low-level-macro-args)
           (error '(quasiquote arguments not a list)))
          ((non-null? (cdr low-level-macro-args))
           (error '(quasiquote arguments more than one or not a list)))
          (else
           (quasiquote-procedure (car low-level-macro-args))))))




;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; List library.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;; singleton.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (singleton x)
  (cons x ()))




;;;; singleton?.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (singleton? x)
  (if (cons? x)
      (null? (cdr x))
      #f))




;;;; null-or-singleton?.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Return non-#f iff the argument is either () or a singleton list.
;;; Rationale: this is useful in macros with a single optional argument,
;;; for checking that the optional part of the arguments has the correct
;;; shape.
(define-constant (null-or-singleton? x)
  (cond ((null? x)
         #t)
        ((cons? x)
         (null? (cdr x)))
        (else
         #f)))




;;;; list?.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; FIXME: a return statement would be nice here.
(define-constant (list?-iterative xs)
  (let* ((go-on #t)
         (res #t))
    (while go-on
      (cond ((null? xs)
             (set! go-on #f))
            ((non-cons? xs)
             (set! res #f)
             (set! go-on #f))
            (else
             (set! xs (cdr xs)))))
    res))

(define-constant (list?-tail-recursive xs)
  (cond ((null? xs)
         #t)
        ((cons? xs)
         (list?-tail-recursive (cdr xs)))
        (else
         #f)))

(define-constant (list? xs)
  (list?-iterative xs))




;;;; symbols?.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Return non-#f iff the argument is a list of symbols, possibly empty.

(define-constant (symbols?-iterative xs)
  (let* ((res #t))
    (while (non-null? xs)
      (cond ((non-cons? xs)
             (set! xs ())
             (set! res #f))
            ((symbol? (car xs))
             (set! xs (cdr xs)))
            (else
             (set! xs ())
             (set! res #f))))
    res))

(define-constant (symbols?-tail-recursive xs)
  (cond ((null? xs)
         #t)
        ((non-cons? xs)
         #f)
        ((symbol? (car xs))
         (symbols?-tail-recursive (cdr xs)))
        (else
         #f)))

(define-constant (symbols? xs)
   (symbols?-iterative xs))




;;;; replicate.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (replicate-iterative n x)
  (let* ((res ()))
    (while (> n 0)
      (set! res (cons x res))
      (set! n (1- n)))
    res))

(define-constant (replicate-tail-recursive-helper n x acc)
  (if (zero? n)
      acc
      (replicate-tail-recursive-helper (1- n) x (cons x acc))))
(define-constant (replicate-tail-recursive n x)
  (replicate-tail-recursive-helper n x ()))

(define-constant (replicate-non-tail-recursive n x)
  (if (zero? n)
      ()
      (cons x (replicate-non-tail-recursive (1- n) x))))

(define-constant (replicate n x)
  (replicate-iterative n x))

;; Just an alias.
(define-constant (make-list n x)
  (replicate n x))




;;;; last-cons.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Return the last cons of xs .  Assume, without checking, that xs is a
;;; non-empty proper list.

(define-constant (last-cons-iterative xs)
  (let* ((cdr-xs (cdr xs)))
    (while (not (null? cdr-xs))
      (set! xs cdr-xs)
      (set! cdr-xs (cdr xs)))
    xs))

(define-constant (last-cons-tail-recursive xs)
  (if (null? (cdr xs))
      xs
      (last-cons-tail-recursive (cdr xs))))

(define-constant (last-cons xs)
  (last-cons-iterative xs))





;;;; last.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (last-iterative xs)
  (car (last-cons-iterative xs)))

(define-constant (last-tail-recursive xs)
  (car (last-cons-tail-recursive xs)))

(define-constant (last xs)
  (last-iterative xs))




;;;; all-but-last-reversed.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (all-but-last-reversed-iterative xs)
  (if (null? xs)
      (error '(all-but-last-reversed-iterative: empty argument))
      (let* ((res ()))
        (while (non-null? (cdr xs))
          (set! res (cons (car xs) res))
          (set! xs (cdr xs)))
        res)))

(define-constant (all-but-last-reversed-non-empty-tail-recursive-helper xs acc)
  (if (null? (cdr xs))
      acc
      (all-but-last-reversed-non-empty-tail-recursive-helper (cdr xs)
                                                             (cons (car xs)
                                                                   acc))))
(define-constant (all-but-last-reversed-tail-recursive xs)
  (if (null? xs)
      (error '(all-but-last-reversed-tail-recursive: empty argument))
      (all-but-last-reversed-non-empty-tail-recursive-helper xs ())))

(define-constant (all-but-last-reversed xs)
  (all-but-last-reversed-iterative xs))




;;;; all-but-last.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (all-but-last-iterative xs)
  (reverse!-iterative (all-but-last-reversed-iterative xs)))

(define-constant (all-but-last-tail-recursive xs)
  (reverse!-tail-recursive (all-but-last-reversed-tail-recursive xs)))

(define-constant (all-but-last-non-empty-non-tail-recursive xs)
  (if (null? (cdr xs))
      ()
      (cons (car xs)
            (all-but-last-non-empty-non-tail-recursive (cdr xs)))))
(define-constant (all-but-last-non-tail-recursive xs)
  (if (null? xs)
      (error '(all-but-last-non-tail-recursive: empty argument))
      (all-but-last-non-empty-non-tail-recursive xs)))

(define-constant (all-but-last xs)
  (all-but-last-iterative xs))




;;;; length.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (length-iterative xs)
  (let* ((res 0))
    (while (non-null? xs)
      (set! res (1+ res))
      (set! xs (cdr xs)))
    res))

(define-constant (length-non-tail-recursive xs)
  (if (null? xs)
      0
      (1+ (length-non-tail-recursive (cdr xs)))))

(define-constant (length-tail-recursive-helper xs acc)
  (if (null? xs)
      acc
      (length-tail-recursive-helper (cdr xs) (1+ acc))))
(define-constant (length-tail-recursive xs)
  (length-tail-recursive-helper xs 0))

(define-constant (length xs)
  (length-iterative xs))




;;;; append-reversed.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (append-reversed-iterative xs ys)
  (let* ((res ys))
    (while (non-null? xs)
      (set! res (cons (car xs) res))
      (set! xs (cdr xs)))
    res))

(define-constant (append-reversed-tail-recursive xs ys)
  (if (null? xs)
      ys
      (append-reversed-tail-recursive (cdr xs)
                                      (cons (car xs) ys))))

(define-constant (append-reversed xs ys)
  (append-reversed-iterative xs ys))




;;;; reverse.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (reverse-iterative xs)
  (append-reversed-iterative xs ()))

;; This uses append-procedure instead of append-non-tail-recursive .
(define-constant (reverse-non-tail-recursive xs)
  (if (null? xs)
      ()
      (append-procedure (reverse-non-tail-recursive (cdr xs))
                        (singleton (car xs)))))

;; This uses append-non-tail-recursive instead of append-procedure .
(define-constant (reverse-really-non-tail-recursive xs)
  (if (null? xs)
      ()
      (append-non-tail-recursive (reverse-non-tail-recursive (cdr xs))
                                 (singleton (car xs)))))

(define-constant (reverse-tail-recursive-helper xs acc)
  (if (null? xs)
      acc
      (reverse-tail-recursive-helper (cdr xs)
                                     (cons (car xs) acc))))
(define-constant (reverse-tail-recursive xs)
  (reverse-tail-recursive-helper xs ()))

(define-constant (reverse xs)
  (reverse-iterative xs))




;;;; reverse!.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (reverse!-iterative xs)
  (if (null? xs)
      ()
      (let* ((previous-list ())
             (the-cons xs)
             (following-list #f))
        (while (not (null? the-cons))
          (set! following-list (cdr the-cons))
          (set-cdr! the-cons previous-list)
          (set! previous-list the-cons)
          (set! the-cons following-list))
        previous-list)))

(define-constant (reverse!-tail-recursive-helper outer-cons non-null-inner-list)
  (let* ((old-non-null-inner-list-cdr (cdr non-null-inner-list)))
    ;; outer-cons: (A . non-null-inner-list) , actually (A . (B . ?))
    ;; non-null-inner-list: (B . ?)
    (set-cdr! non-null-inner-list outer-cons)
    (if (null? old-non-null-inner-list-cdr)
        non-null-inner-list
        (reverse!-tail-recursive-helper non-null-inner-list
                                        old-non-null-inner-list-cdr))))
(define-constant (reverse!-tail-recursive xs)
  (if (null? xs)
      ()
      (reverse!-tail-recursive-helper () xs)))

(define-constant (reverse! xs)
  (reverse!-iterative xs))




;;;; append.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (append-iterative xs ys)
  (append-reversed-iterative (reverse-iterative xs) ys))

(define-constant (append-non-tail-recursive xs ys)
  (if (null? xs)
      ys
      (cons (car xs)
            (append-non-tail-recursive (cdr xs) ys))))

(define-constant (append-tail-recursive xs ys)
  (append-reversed-tail-recursive (reverse-tail-recursive xs)
                                  ys))

(define-constant (append-procedure xs ys)
  (append-iterative xs ys))




;;;; append!.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (append!-iterative xs ys)
  (if (null? xs)
      ys
      (begin
        (set-cdr! (last-cons-iterative xs) ys)
        xs)))

(define-constant (append!-tail-recursive xs ys)
  (if (null? xs)
      ys
      (begin
        (set-cdr! (last-cons-tail-recursive xs) ys)
        xs)))

(define-constant (append!-procedure xs ys)
  (append!-iterative xs ys))




;;;; flatten-reversed.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (flatten-reversed-iterative list-of-lists)
  (let* ((res ()))
    (while (non-null? list-of-lists)
      (set! res (append-iterative (car list-of-lists) res))
      (set! list-of-lists (cdr list-of-lists)))
    res))

(define-constant (flatten-reversed-tail-recursive-helper reversed-list acc)
  (if (null? reversed-list)
      acc
      (flatten-reversed-tail-recursive-helper
          (cdr reversed-list)
          (append-tail-recursive (car reversed-list) acc))))
(define-constant (flatten-reversed-tail-recursive reversed-list)
  (flatten-reversed-tail-recursive-helper reversed-list ()))

(define-constant (flatten-reversed reversed-list)
  (flatten-reversed-iterative reversed-list))




;;;; flatten-reversed!.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (flatten-reversed!-iterative list-of-lists)
  (let* ((res ()))
    (while (non-null? list-of-lists)
      (set! res (append!-iterative (car list-of-lists) res))
      (set! list-of-lists (cdr list-of-lists)))
    res))

(define-constant (flatten-reversed! list-of-lists)
  (flatten-reversed!-iterative list-of-lists))




;;;; flatten.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (flatten-iterative list-of-lists)
  (flatten-reversed-iterative (reverse-iterative list-of-lists)))

(define-constant (flatten-tail-recursive list-of-lists)
  (flatten-reversed-tail-recursive (reverse-tail-recursive list-of-lists)))

(define-constant (flatten-non-tail-recursive list-of-lists)
  (if (null? list-of-lists)
      ()
      (append-non-tail-recursive
          (car list-of-lists)
          (flatten-non-tail-recursive (cdr list-of-lists)))))

(define-constant (flatten list-of-lists)
  (flatten-iterative list-of-lists))




;;;; flatten!.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (flatten!-iterative list-of-lists)
  (flatten-reversed!-iterative (reverse!-iterative list-of-lists)))

(define-constant (flatten!-tail-recursive list-of-lists)
  (flatten-reversed-tail-recursive (reverse!-tail-recursive list-of-lists)))

(define-constant (flatten!-non-tail-recursive list-of-lists)
  (if (null? list-of-lists)
      ()
      ;; I don't have an append!-non-tail-recursive, as it doesn't seem very
      ;; reasonable.
      (append!-procedure (car list-of-lists)
                         (flatten!-non-tail-recursive (cdr list-of-lists)))))

(define-constant (flatten! list-of-lists)
  (flatten!-iterative list-of-lists))




;;;; list-copy.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; These return a shallow-copy of the given list: only the spine is
;;; duplicated.

(define-constant (list-copy-iterative xs)
  (if (null? xs)
      ()
      (let* ((res (cons (car xs) #f))
             (last-cons res)
             (new-cons #f))
        (set! xs (cdr xs))
        (while (non-null? xs)
          (set! new-cons (cons (car xs) #f))
          (set-cdr! last-cons new-cons)
          (set! last-cons new-cons)
          (set! xs (cdr xs)))
        (set-cdr! last-cons ())
        res)))

(define-constant (list-copy-tail-recursive xs)
  (reverse!-tail-recursive (reverse-tail-recursive xs)))

(define-constant (list-copy-non-tail-recursive xs)
  (if (null? xs)
      ()
      (cons (car xs)
            (list-copy-non-tail-recursive (cdr xs)))))

(define-constant (list-copy xs)
  (list-copy-iterative xs))




;;;; car-or-nil.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (car-or-nil xs)
  (if (null? xs)
      ()
      (car xs)))




;;;; cdr-or-nil.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (cdr-or-nil xs)
  (if (null? xs)
      ()
      (cdr xs)))




;;;; nth-cons-or-nil.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (nth-cons-or-nil-iterative xs n)
  ;; A break or return form would be useful here.  Even better I could also
  ;; iterate on an and condition, but we have no and macro yet.
  (let* ((go-on #t))
    (while go-on
      (cond ((zero? n)
             (set! go-on #f))
            ((null? xs)
             (set! go-on #f))
            (else
             (set! n (1- n))
             (set! xs (cdr xs)))))
    xs))

(define-constant (nth-cons-or-nil-tail-recursive xs n)
  (cond ((zero? n)
         xs)
        ((null? xs)
         ())
        (else
         (nth-cons-or-nil-tail-recursive (cdr xs) (1- n)))))

(define-constant (nth-cons-or-nil xs n)
  (nth-cons-or-nil-iterative xs n))




;;;; nth-cons.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (nth-cons xs n)
  (let* ((c (nth-cons-or-nil n xs)))
    (if (null? c)
        (error '(nth-cons: list too short))
        c)))




;;;; nth.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (nth-iterative xs n)
  (while (non-zero? n)
    (set! xs (cdr xs))
    (set! n (1- n)))
  (car xs))

(define-constant (nth-tail-recursive xs n)
  (if (zero? n)
      (car xs)
      (nth-tail-recursive (cdr xs) (1- n))))

(define-constant (nth xs n)
  (nth-iterative xs n))




;;;; take, take-reversed.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; A break or return form would be useful here.
(define-constant (take-reversed-iterative xs n)
  (let* ((res ())
         (go-on #t))
    (while go-on
      (cond ((null? xs)
             (set! go-on #f))
            ((zero? n)
             (set! go-on #f))
            (else
             (set! res (cons (car xs) res))
             (set! xs (cdr xs))
             (set! n (1- n)))))
    res))
(define-constant (take-iterative xs n)
  (reverse!-iterative (take-reversed-iterative xs n)))

(define-constant (take-tail-recursive xs n)
  (reverse!-tail-recursive (take-reversed-tail-recursive xs n)))

(define-constant (take-non-tail-recursive xs n)
  (cond ((zero? n)
         ())
        ((null? xs)
         ())
        (else
         (cons (car xs) (take-non-tail-recursive (cdr xs) (1- n))))))

(define-constant (take-reversed-tail-recursive-helper xs n acc)
  (cond ((zero? n)
         acc)
        ((null? xs)
         acc)
        (else
         (take-reversed-tail-recursive-helper (cdr xs)
                                              (1- n)
                                              (cons (car xs) acc)))))
(define-constant (take-reversed-tail-recursive xs n)
  (take-reversed-tail-recursive-helper xs n ()))

(define-constant (take-reversed xs n)
  (take-reversed-iterative xs n))

(define-constant (take xs n)
  (take-iterative xs n))




;;;; take!
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (take!-iterative xs n)
  (if (zero? n)
      ()
      (let* ((n-1-th-cons-or-nil (nth-cons-or-nil-iterative xs (1- n))))
        (if (non-null? n-1-th-cons-or-nil)
            (set-cdr! n-1-th-cons-or-nil ()))
        xs)))

(define-constant (take!-tail-recursive-helper xs n)
  (cond ((= n 1)
         (if (null? xs)
             'do-nothing
             (set-cdr! xs ())))
        ((null? xs))
        (else
         (take!-tail-recursive-helper (cdr xs) (1- n)))))
(define-constant (take!-tail-recursive xs n)
  (if (zero? n)
      ()
      (begin
        (take!-tail-recursive-helper xs n)
        xs)))

(define-constant (take! xs n)
  (take!-iterative xs n))




;;;; drop.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; A break or return form would be useful here.
(define-constant (drop-iterative xs n)
  (let* ((go-on #t))
    (while go-on
      (cond ((null? xs)
             (set! go-on #f))
            ((zero? n)
             (set! go-on #f))
            (else
             (set! xs (cdr xs))
             (set! n (1- n)))))
    xs))

(define-constant (drop-tail-recursive xs n)
  (cond ((zero? n)
         xs)
        ((null? xs)
         ())
        (else
         (drop-tail-recursive (cdr xs) (1- n)))))

(define-constant (drop xs n)
  (drop-iterative xs n))




;;;; drop!.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (drop! xs n)
  (if (zero? n)
      xs
      (let* ((n-1-th-cons-or-nil (nth-cons-or-nil xs (1- n))))
        (if (non-null? n-1-th-cons-or-nil)
            (let* ((old-cdr (cdr n-1-th-cons-or-nil)))
              (set-cdr! n-1-th-cons-or-nil ())
              old-cdr)
            ()))))




;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; High-level macros.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; At this point I have a good implementation for the fundamental list
;;; procedures, which lets me use quasiquoting in macros.



;;;; destructuring-bind-procedure.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Rationale: Every low-level macro has exactly *one* formal parameter, always
;;; named low-level-macro-args and bound to the entire macro call cdr.  Thru
;;; destructuring-bind we can turn convenient high-level macros with named
;;; formals into low-level macros, by binding each formal to a
;;; low-level-macro-args component.
;;;
;;; The mapping from high-level to low-level macros is epsilon-style; at least,
;;; I designed it the first time for epsilon.  I wouldn't be very surprised if
;;; somebody had the same idea before, which may be the reason why
;;; destructuring-bind exists in Common Lisp -- I discovered it after devising
;;; my own mechanism, which originally had a different name but behaved
;;; identically.
;;; Update: the answer may be in http://www.lispworks.com/documentation/HyperSpec/Issues/iss130_w.htm
;;; I've discovered Emacs Lisp's seq-let , less powerful but conceptually very
;;; similar, only now in 2017.

;;; Return the result of destructuring-bind with the given formal pattern bound
;;; to low-level-macro-args or some sub-component of it.  The result is code,
;;; not executed by this function: of course we cannot do that until we have an
;;; actual value for low-level-macro-args .
;;; Notice that component may be evaluated multiple times in the returned code,
;;; and therefore should be a literal or a variable.  This is ensured out of
;;; this recursive procedure, by calling it with an appropriate actual.
;;; Common Lisp calls "template" what we call "pattern" here.
(define-constant (destructuring-bind-recursive formals-pattern
                                               component
                                               body-forms)
  (cond ((null? formals-pattern)
         ;; There is nothing to bind in the pattern.  Return code to check
         ;; that there are also no actuals, and then either proceeds or fails.
         `(if (null? ,component)
              (begin
                ,@body-forms)
              (error `(destructuring-bind: excess actuals: ,,component))))
        ((symbol? formals-pattern)
         ;; The macro pattern is dotted, or this is a recursive call on a
         ;; pattern car: in either case bind one variable to every actual.
         `(let* ((,formals-pattern ,component))
            ,@body-forms))
        ((cons? formals-pattern)
         ;; Bind both the car and the cdr.  For efficiency's sake name the two
         ;; sub-components in the generated code.
         (let* ((car-name (gensym))
                (cdr-name (gensym)))
           `(let* ((,car-name (car ,component))
                   (,cdr-name (cdr ,component)))
              ,(destructuring-bind-recursive
                  (car formals-pattern)
                  car-name
                  ;; The inner quasiquoting serves to make a (singleton) list of
                  ;; the body forms.
                  `(,(destructuring-bind-recursive (cdr formals-pattern)
                                                   cdr-name
                                                   body-forms))))))
        ((vector? formals-pattern)
         (error `(vector ,formals-pattern in macro formals pattern)))
        (else
         ;; The pattern is, hopefully, something which can be compared with eq?
         ;; .  Return code checking that it's equal to the actual and in that
         ;; case proceeds without binding anything.
         `(if (eq? ,formals-pattern ,component)
              (begin
                ,@body-forms)
              (error `(non-matching pattern argument: ,formals-pattern
                                    ,component))))))

;;; The args argument represents "actuals" in a symbolic form; their values
;;; may not necessarily be known yet.
;;; Example:
;;; (destructuring-bind-procedure
;;;   '(a b)
;;;   'some-arguments
;;;   '((display a) (display b)))
;;; This would return code binding a and b as local variable to the car and
;;; cadr of some-arguments, assumed to be bound, and display them.
(define-constant (destructuring-bind-procedure formals-pattern args body-forms)
  (let* ((args-value-name (gensym)))
    `(let* ((,args-value-name ,args))
       ,(destructuring-bind-recursive formals-pattern
                                      args-value-name
                                      body-forms))))

;; FIXME: check that the formals-pattern doesn't require non-linear bindings.




;;;; destructuring-bind.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; This will be convenient to define high-level macros on top of low-level
;;; macros, by destructuring the one low-level macro argument.

;;; Arguments: pattern structure . body-forms Evaluate structure and locally
;;; bind its components with the variables in the pattern; return the result of
;;; evaluating the body forms with the bindings visible.
(define-constant destructuring-bind
  (low-level-macro
    (let* ((pattern (car low-level-macro-args))
           (structure (cadr low-level-macro-args))
           (body-forms (cddr low-level-macro-args)))
      (destructuring-bind-procedure pattern structure body-forms))))




;;;; High-level macros.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Expand to a form evaluating to a high-level macro.
;;; Arguments: formals . body-forms
;;; Scheme-style, where formals can be an improper list; however there is no
;;; macro name here.
(define-constant macro
  (low-level-macro
    (let* ((macro-formals (car low-level-macro-args))
           (macro-body-forms (cdr low-level-macro-args)))
      `(low-level-macro
         (destructuring-bind ,macro-formals
                             low-level-macro-args
                             ,@macro-body-forms)))))

;;; Globally define a high-level named macro.
;;; Arguments: (name . pattern) . body-forms
;;; The pattern is of the form accepted by destructuring-bind.
(define-constant define-macro
  (macro ((macro-name . macro-formals) . macro-body-forms)
    `(define ,macro-name
       (macro ,macro-formals ,@macro-body-forms))))

;;; High-level macros are now usable.




;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; More advanced list library.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;; alist functions.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (alist? x)
  (cond ((null? x)
         #t)
        ((non-cons? x)
         #f)
        ((cons? (car x))
         (alist? (cdr x)))
        (else
         #f)))

(define-constant (assq key alist)
  (cond ((null? alist)
         #f)
        ((eq? (caar alist) key)
         (car alist))
        (else
         (assq key (cdr alist)))))

(define-constant (rassq value alist)
  (cond ((null? alist)
         #f)
        ((eq? (cdar alist) value)
         (car alist))
        (else
         (rassq value (cdr alist)))))

;;; Return a new alist, possibly sharing structure with alist, without the
;;; first binding of the given object, if any.
(define-constant (del-assq-1-noncopying object alist)
  (cond ((null? alist)
         ())
        ((eq? (caar alist) object)
         (cdr alist))
        (else
         (cons (car alist) (del-assq-1-noncopying object (cdr alist))))))

(define-constant (del-assq-1 object alist)
  (del-assq-1-noncopying object (list-copy alist)))

(define-constant (del-assq-noncopying object alist)
  (cond ((null? alist)
         ())
        ((eq? (caar alist) object)
         (del-assq-noncopying object (cdr alist)))
        (else
         (cons (car alist) (del-assq-noncopying object (cdr alist))))))

(define-constant (del-assq object alist)
  (del-assq-noncopying object (list-copy alist)))

;;; An obvious extension of del-assq, returning a copy of the alist with the
;;; bindings for all of the given keys removed.
(define-constant (del-assq-list-noncopying objects alist)
  (if (null? objects)
      alist
      (del-assq-list-noncopying (cdr objects)
                                (del-assq-noncopying (car objects) alist))))

(define-constant (del-assq-list objects alist)
  (del-assq-list-noncopying objects (list-copy alist)))

;; FIXME: implement del-assq! .

(define-constant (alist-copy alist)
  (let* ((res ())
         (first-cons #f))
    (while (non-null? alist)
      (set! first-cons (car alist))
      (set! res (cons (cons (car first-cons) (cdr first-cons)) res))
      (set! alist (cdr alist)))
    (reverse! res)))

(define-constant (alist-get key alist)
  (let* ((a-cons (assq key alist)))
    (if (cons? a-cons)
        (cdr a-cons)
        (error `(alist-get: key ,key not found in alist ,alist)))))




;;;; zip-reversed.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (zip-reversed-iterative as bs)
  (let* ((res ()))
    (while (non-null? as)
      (if (null? bs)
          (error '(zip-reversed-iterative: first list longer))
          (begin
            (set! res (cons (cons (car as) (car bs))
                            res))
            (set! as (cdr as))
            (set! bs (cdr bs)))))
    (if (non-null? bs)
        (error '(zip-reversed-iterative: second list longer)))
    res))

(define-constant (zip-reversed-tail-recursive-helper as bs acc)
  (cond ((null? as)
         (if (non-null? bs)
             (error '(zip-non-tail-recursive: second list longer))
             acc))
        ((null? bs)
         (error '(zip-tail-recursive: first list longer)))
        (else
         (zip-reversed-tail-recursive-helper (cdr as)
                                             (cdr bs)
                                             (cons (cons (car as)
                                                         (car bs))
                                                   acc)))))
(define-constant (zip-reversed-tail-recursive as bs)
  (zip-reversed-tail-recursive-helper as bs ()))

(define-constant (zip-reversed as bs)
  (zip-reversed-iterative as bs))




;;;; zip.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (zip-iterative as bs)
  (reverse! (zip-reversed-iterative as bs)))

(define-constant (zip-tail-recursive as bs)
  (reverse!-tail-recursive (zip-reversed-tail-recursive as bs)))

(define-constant (zip-non-tail-recursive as bs)
  (cond ((null? as)
         (if (non-null? bs)
             (error '(zip-non-tail-recursive: second list longer))
             ()))
        ((null? bs)
         (error '(zip-non-tail-recursive: first list longer)))
        (else
         (cons (cons (car as) (car bs))
               (zip-non-tail-recursive (cdr as) (cdr bs))))))

(define-constant (zip as bs)
  (zip-iterative as bs))




;;;; unzip-reversed.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (unzip-reversed-iterative as)
  (let* ((cars ())
         (cdrs ()))
    (while (non-null? as)
      (set! cars (cons (caar as) cars))
      (set! cdrs (cons (cdar as) cdrs))
      (set! as (cdr as)))
    (cons cars cdrs)))

(define-constant (unzip-reversed-tail-recursive-helper as acc-cars acc-cdrs)
  (if (null? as)
      (cons acc-cars acc-cdrs)
      (unzip-reversed-tail-recursive-helper (cdr as)
                                            (cons (caar as) acc-cars)
                                            (cons (cdar as) acc-cdrs))))
(define-constant (unzip-reversed-tail-recursive as)
  (unzip-reversed-tail-recursive-helper as () ()))

(define-constant (unzip-reversed as)
  (unzip-reversed-iterative as))




;;;; unzip.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (unzip-iterative as)
  (let* ((cons-reversed (unzip-reversed-iterative as)))
    (cons (reverse! (car cons-reversed))
          (reverse! (cdr cons-reversed)))))

(define-constant (unzip-tail-recursive as)
  (let* ((cons-reversed (unzip-reversed-tail-recursive as)))
    (cons (reverse! (car cons-reversed))
          (reverse! (cdr cons-reversed)))))

(define-constant (unzip-non-tail-recursive as)
  (if (null? as)
      '(() . ())
      (let* ((unzipped-cdr (unzip-non-tail-recursive (cdr as))))
        (cons (cons (caar as) (car unzipped-cdr))
              (cons (cdar as) (cdr unzipped-cdr))))))

(define-constant (unzip as)
  (unzip-iterative as))




;;;; map!.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (map!-iterative f xs)
  (let* ((res xs))
    (while (non-null? xs)
      (set-car! xs (f (car xs)))
      (set! xs (cdr xs)))
    res))

(define-constant (map!-tail-recursive-helper f xs)
  (if (null? xs)
      'done
      (begin
        (set-car! xs (f (car xs)))
        (map!-tail-recursive-helper f (cdr xs)))))
(define-constant (map!-tail-recursive f xs)
  (map!-tail-recursive-helper f xs)
  xs)

(define-constant (map! f xs)
  (map!-iterative f xs))




;;;; map-reversed.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (map-reversed-iterative f xs)
  (let* ((res ()))
    (while (non-null? xs)
      (set! res (cons (f (car xs)) res))
      (set! xs (cdr xs)))
    res))

(define-constant (map-reversed-tail-recursive-helper f xs acc)
  (if (null? xs)
      acc
      (map-reversed-tail-recursive-helper f
                                          (cdr xs)
                                          (cons (f (car xs))
                                                acc))))
(define-constant (map-reversed-tail-recursive f xs)
  (map-reversed-tail-recursive-helper f xs ()))

(define-constant (map-reversed f xs)
  (map-reversed-iterative f xs))




;;;; map.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (map-iterative f xs)
  (reverse!-iterative (map-reversed-iterative f xs)))

(define-constant (map-non-tail-recursive f xs)
  (if (null? xs)
      ()
      (cons (f (car xs))
            (map-non-tail-recursive f (cdr xs)))))

(define-constant (map-tail-recursive f xs)
  (reverse!-tail-recursive (map-reversed-tail-recursive f xs)))

(define-constant (map f xs)
  (map-iterative f xs))




;;;; fold-left.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (fold-left-iterative f x xs)
  (while (non-null? xs)
    (set! x (f x (car xs)))
    (set! xs (cdr xs)))
  x)

(define-constant (fold-left-iterative-reversed-f reversed-f x xs)
  (while (non-null? xs)
    (set! x (reversed-f (car xs) x))
    (set! xs (cdr xs)))
  x)

(define-constant (fold-left-tail-recursive f x xs)
  (if (null? xs)
      x
      (fold-left-tail-recursive f
                                (f x (car xs))
                                (cdr xs))))

(define-constant (fold-left-tail-recursive-reversed-f reversed-f x xs)
  (if (null? xs)
      x
      (fold-left-tail-recursive-reversed-f reversed-f
                                           (reversed-f (car xs) x)
                                           (cdr xs))))

(define-constant (fold-left f x xs)
  (fold-left-iterative f x xs))




;;;; fold-right.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (fold-right-iterative f xs y)
  (fold-left-iterative-reversed-f f
                                  y
                                  (reverse-iterative xs)))

(define-constant (fold-right-tail-recursive f xs y)
  (fold-left-tail-recursive-reversed-f f
                                       y
                                       (reverse-tail-recursive xs)))

(define-constant (fold-right-non-tail-recursive f xs y)
  (if (null? xs)
      y
      (f (car xs)
         (fold-right-non-tail-recursive f (cdr xs) y))))

(define-constant (fold-right f xs y)
  (fold-right-iterative f xs y))




;;;; fold-right!.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (fold-right!-iterative f xs y)
  (fold-left-iterative-reversed-f f
                                  y
                                  (reverse!-iterative xs)))

(define-constant (fold-right! f xs y)
  (fold-right!-iterative f xs y))




;;;; exists?.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; FIXME: a return form would also be nice here.
(define-constant (exists?-iterative p xs)
  (let* ((res #f)
         (go-on #t))
    (while go-on
      (cond ((null? xs)
             (set! go-on #f))
            ((p (car xs))
             (begin
               (set! res #t)
               (set! go-on #f)))
            (else
             (set! xs (cdr xs)))))
    res))

(define-constant (exists?-tail-recursive p xs)
  (cond ((null? xs)
         #f)
        ((p (car xs))
         #t)
        (else
         (exists?-tail-recursive p (cdr xs)))))

(define-constant (exists? p xs)
  (exists?-iterative p xs))




;;;; for-all?.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; FIXME: a return form would be nice here.
(define-constant (for-all?-iterative p xs)
  (let* ((res #t)
         (go-on #t))
    (while go-on
      (cond ((null? xs)
             (set! go-on #f))
            ((not (p (car xs)))
             (begin
               (set! res #f)
               (set! go-on #f)))
            (else
             (set! xs (cdr xs)))))
    res))

(define-constant (for-all?-tail-recursive p xs)
  (cond ((null? xs)
         #t)
        ((p (car xs))
         (for-all?-tail-recursive p (cdr xs)))
        (else
         #f)))

(define-constant (for-all? p xs)
  (for-all?-iterative p xs))


;;;; filter, filter-reversed.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (filter-reversed-iterative p xs)
  (let* ((res ()))
    (while (non-null? xs)
      (if (p (car xs))
          (set! res (cons (car xs) res))
          'do-nothing)
      (set! xs (cdr xs)))
    res))

(define-constant (filter-iterative p xs)
  (reverse!-iterative (filter-reversed-iterative p xs)))

(define-constant (filter-non-tail-recursive p xs)
  (cond ((null? xs)
         ())
        ((p (car xs))
         (cons (car xs) (filter-non-tail-recursive p (cdr xs))))
        (else
         (filter-non-tail-recursive p (cdr xs)))))

(define-constant (filter-reversed-tail-recursive-helper p xs acc)
  (cond ((null? xs)
         acc)
        ((p (car xs))
         (filter-reversed-tail-recursive-helper p (cdr xs) (cons (car xs) acc)))
        (else
         (filter-reversed-tail-recursive-helper p (cdr xs) acc))))
(define-constant (filter-reversed-tail-recursive p xs)
  (filter-reversed-tail-recursive-helper p xs ()))

(define-constant (filter-tail-recursive p xs)
  (reverse!-tail-recursive (filter-reversed-tail-recursive p xs)))

(define-constant (filter-reversed p xs)
  (filter-reversed-tail-recursive p xs))

(define-constant (filter p xs)
  (filter-tail-recursive p xs))





;;;; range-reversed.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (range-reversed-iterative a b)
  (let* ((res ()))
    (while (<= a b)
      (set! res (cons a res))
      (set! a (1+ a)))
    res))

(define-constant (range-reversed-non-tail-recursive a b)
  (if (> a b)
      ()
      (cons b (range-reversed-non-tail-recursive a (1- b)))))

(define-constant (range-reversed-tail-recursive-helper a b acc)
  (if (> a b)
      acc
      (range-reversed-tail-recursive-helper (1+ a) b (cons a acc))))
(define-constant (range-reversed-tail-recursive a b)
  (range-reversed-tail-recursive-helper a b ()))

(define-constant (range-reversed a b)
  (range-reversed-iterative a b))




;;;; range.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (range-iterative a b)
  (let* ((res ()))
    (while (<= a b)
      (set! res (cons b res))
      (set! b (1- b)))
    res))

(define-constant (range-non-tail-recursive a b)
  (if (> a b)
      ()
      (cons a (range-non-tail-recursive (1+ a) b))))

(define-constant (range-tail-recursive-helper a b acc)
  (if (> a b)
      acc
      (range-tail-recursive-helper a (1- b) (cons b acc))))
(define-constant (range-tail-recursive a b)
  (range-tail-recursive-helper a b ()))

(define-constant (range a b)
  (range-iterative a b))




;;;; iota.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (iota-iterative n)
  (let* ((res ()))
    (while (> n 0)
      (set! n (1- n))
      (set! res (cons n res)))
    res))

(define-constant (iota-tail-recursive-helper n acc)
  (if (< n 0)
      acc
      (iota-tail-recursive-helper (1- n) (cons n acc))))
(define-constant (iota-tail-recursive n)
  (iota-tail-recursive-helper (1- n) ()))

(define-constant (iota-non-tail-recursive n)
  (range-non-tail-recursive 0 (1- n)))

(define-constant (iota n)
  (iota-iterative n))




;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; Sorting.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;; insert.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; FIXME: a return or break form would be nice here.
(define-constant (insert-iterative x xs)
  (let* ((smaller-elements-reversed ())
         (go-on #t))
    (while go-on
      (cond ((null? xs)
             (set! go-on #f))
            ((<= x (car xs))
             (set! go-on #f))
            (else
             (set! smaller-elements-reversed
                   (cons (car xs) smaller-elements-reversed))
             (set! xs (cdr xs)))))
    (append-reversed-iterative smaller-elements-reversed
                               (cons x xs))))

(define-constant (insert-tail-recursive-helper x xs smaller-elements-reversed)
  (cond ((null? xs)
         (append-reversed-tail-recursive smaller-elements-reversed
                                         (singleton x)))
        ((<= x (car xs))
         (append-reversed-tail-recursive smaller-elements-reversed
                                         (cons x xs)))
        (else
         (insert-tail-recursive-helper x
                                       (cdr xs)
                                       (cons (car xs)
                                             smaller-elements-reversed)))))
(define-constant (insert-tail-recursive x xs)
  (insert-tail-recursive-helper x xs ()))

(define-constant (insert-non-tail-recursive x xs)
  (cond ((null? xs)
         (singleton x))
        ((<= x (car xs))
         (cons x xs))
        (else
         (cons (car xs)
               (insert-non-tail-recursive x (cdr xs))))))

(define-constant (insert x xs)
  (insert-iterative x xs))




;;;; insert!.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Destructively turn (A . B) into (x . (A . B)).
(define-constant (insert-as-first! x a-cons)
  (let* ((new-cons (cons (car a-cons) (cdr a-cons))))
    (set-car! a-cons x)
    (set-cdr! a-cons new-cons)))

;;; Destructively turn (A . B) into (A . (x . B)).
(define-constant (insert-as-second! x a-cons)
  (let* ((new-cons (cons x (cdr a-cons))))
    (set-cdr! a-cons new-cons)))

(define-constant (insert!-iterative-non-null x xs)
  (let* ((go-on #t))
    (while go-on
      ;; Here I can still assume that xs is not ().
      (cond ((< x (car xs))
             (begin
               (insert-as-first! x xs)
               (set! go-on #f)))
            ((null? (cdr xs))
             (begin
               (insert-as-second! x xs)
               (set! go-on #f)))
            (else
             (set! xs (cdr xs)))))))
(define-constant (insert!-iterative x xs)
  (if (null? xs)
      (singleton x)
      (begin
        (insert!-iterative-non-null x xs)
        xs)))

(define-constant (insert!-tail-recursive-non-null x xs)
  (cond ((< x (car xs))
         (insert-as-first! x xs))
        ((null? (cdr xs))
         (insert-as-second! x xs))
        (else
         (insert!-tail-recursive-non-null x (cdr xs)))))
(define-constant (insert!-tail-recursive x xs)
  (if (null? xs)
      (singleton x)
      (begin
        (insert!-tail-recursive-non-null x xs)
        xs)))

(define-constant (insert! x xs)
  (insert!-iterative x xs))




;;;; insertion-sort.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (insertion-sort-iterative xs)
  (let* ((res ()))
    (while (non-null? xs)
      (set! res (insert!-iterative (car xs) res))
      (set! xs (cdr xs)))
    res))

(define-constant (insertion-sort-tail-recursive-helper xs acc)
  (if (null? xs)
      acc
      (insertion-sort-tail-recursive-helper (cdr xs)
                                            (insert!-tail-recursive (car xs)
                                                                    acc))))
(define-constant (insertion-sort-tail-recursive xs)
  (insertion-sort-tail-recursive-helper xs ()))

(define-constant (insertion-sort xs)
  (insertion-sort-iterative xs))




;;;; sort.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (sort xs)
  (insertion-sort xs))




;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; Higher order.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;; identity.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (identity x)
  x)




;;;; compose.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (compose-procedure f g)
  (lambda (x) (f (g x))))

(define-constant (compose-eta f g x)
  (f (g x)))

(define-constant (square-function f)
  (lambda (x) (f (f x))))

(define-constant (square-function-eta f x)
  (f (f x)))




;;;; iterate.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (iterate-iterative-post f n)
  (lambda (x)
    (let* ((n n))
      (while (> n 0)
        (set! x (f x))
        (set! n (1- n)))
      x)))

(define-constant (iterate-iterative-pre f n)
  (let* ((res identity))
    (while (> n 0)
      (set! res (compose-procedure res f))
      (set! n (1- n)))
    res))

(define-constant (iterate-eta f n x)
  (if (zero? n)
      x
      (iterate-eta f (1- n) (f x))))
(define-constant (iterate-tail-recursive-post f n)
  (lambda (x) (iterate-eta f n x)))

;; This builds a function which is even faster than the iterative versions (at
;; least on the naïf JitterLisp interpreter).
(define-constant (iterate-squaring-pre f n)
  ;; This uses the same idea of exponentiation by squaring; the advantage here
  ;; is the very small number of built closures, only O(lg n).  Recursion depth
  ;; is also logarithmic, so non-tail calls are not a problem here.
  (cond ((zero? n)
         identity)
        ((= n 1)
         ;; An important case to optimize, in order to avoid compositions
         ;; between f and the identity, which would be executed many times
         ;; when the iterated function is eventually called.
         f)
        ((even? n)
         (let* ((f^n/2 (iterate-squaring-pre f (quotient n 2))))
           (square-function f^n/2)))
        (else
         (compose-procedure f (iterate-squaring-pre f (1- n))))))

(define-constant (iterate-squaring-eta f n x)
  ;; This uses the same idea of exponentiation by squaring; the advantage here
  ;; is the very small number of built closures, only O(lg n).
  (cond ((zero? n)
         x)
        ((even? n)
         (iterate-squaring-eta (square-function f) (quotient n 2) x))
        (else
         (iterate-squaring-eta (square-function f) (quotient n 2) (f x)))))
(define-constant (iterate-squaring-post f n)
  (lambda (x) (iterate-squaring-eta f n x)))

(define-constant (iterate-tail-recursive-pre-helper f n acc)
  (if (zero? n)
      acc
      (iterate-tail-recursive-pre-helper f (1- n) (compose-procedure acc f))))
(define-constant (iterate-tail-recursive-pre f n)
  (iterate-tail-recursive-pre-helper f n identity))

(define-constant (iterate-pre f n)
  (iterate-squaring-pre f n))
(define-constant (iterate-post f n)
  (iterate-squaring-post f n))
(define-constant (iterate f n)
  (iterate-squaring-pre f n))




;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; Stuff to move.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; The code below is okay, but should be moved up.


;;;; list-has?.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Return non-#f iff x is eq? to at least one of the elements of xs, assumed
;;; to be a list.
;; (define-constant (list-has? xs x)
;;   (let* ((res #f)
;;          (done #f)) ;; A break or return form would be nice here.
;;     (while (not done)
;;       (cond ((null? xs)
;;              (set! done #t))
;;             ((eq? (car xs) x)
;;              (set! res #t)
;;              (set! done #t))
;;             (else
;;              (set! xs (cdr xs)))))
;;     res))

(define-constant (non-empty-list-has? xs x)
  (let* ((res #f))
    (while (not (null? xs))
      (if (eq? (car xs) x)
          (begin
            (set! res #t)
            (set! xs ()))
          (set! xs (cdr xs))))
    res))
(define-constant (list-has? xs x)
  (if (null? xs)
      #f
      (non-empty-list-has? xs x)))




;;;; list-without.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Return a copy of the list xs, which may share structure with it, without the
;;; first element which is eq? to x (if any), in the same order.

(define-constant (list-without-iterative xs x)
  (let* ((reversed-prefix ())
         (go-on #t)) ;; A return statement would be nice.
    (while go-on
      (cond ((null? xs)
             (set! go-on #f))
            ((eq? (car xs) x)
             (set! xs (cdr xs))
             (set! go-on #f))
            (else
             (set! reversed-prefix (cons (car xs) reversed-prefix))
             (set! xs (cdr xs)))))
    (append!-iterative (reverse!-iterative reversed-prefix) xs)))

(define-constant (list-without-tail-recursive-helper xs x acc)
  (cond ((null? xs)
         (reverse!-tail-recursive acc))
        ((eq? (car xs) x)
         (append!-tail-recursive (reverse!-tail-recursive acc)
                                 (cdr xs)))
        (else
         (list-without-tail-recursive-helper (cdr xs)
                                             x
                                             (cons (car xs) acc)))))
(define-constant (list-without-tail-recursive xs x)
  (list-without-tail-recursive-helper xs x ()))

(define-constant (list-without-non-tail-recursive xs x)
  (cond ((null? xs)
         ())
        ((eq? (car xs) x)
         (cdr xs))
        (else
         (cons (car xs)
               (list-without-non-tail-recursive (cdr xs) x)))))

(define-constant (list-without xs x)
  (list-without-iterative xs x))




;;;; list-without!.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Return the first cons of the list xs whose cadr is eq? to x, or () if no such
;; cons exists.  In other word, return the cons containing the predecessor
;; element of x in xs, or the list suffix whose second element is eq? to x.
;; Assume that xs has at least one element.
(define-constant (cons-before-or-nil xs x)
  (let* ((go-on #t) ;; A break or return form would be nice here.
         (next-cons (cdr xs)))
    (while go-on
      (cond ((null? next-cons)
             (set! xs ())
             (set! go-on #f))
            ((eq? (car next-cons) x)
             (set! go-on #f))
            (else
             (set! xs next-cons)
             (set! next-cons (cdr next-cons)))))
    ;; Now xs is either the predecessor we were looking for or ().
    xs))

;;; Return a list equal to xs except that the first element eq? to x, if any,
;;; has been removed.  This may destructively modify xs, and the result may
;;; share structure with it.
(define-constant (list-without! xs x)
  (cond ((null? xs)
         ;; Empty list: there is nothing to remove.
         ())
        ((null? (cdr xs))
         ;; Singleton: return an empty list or xs unchanged.
         (if (eq? (car xs) x)
             ()
             xs))
        ((eq? (car xs) x)
         ;; Special case: if the equal element is the first then we cannot
         ;; modify xs to turn it into the result, but we already have the
         ;; result as a substructure of xs.
         (cdr xs))
        (else
         ;; Here we may have the opportunity to actually modify the list.
         ;; Find the predecessor cons, if any.
         (let* ((predecessor-or-nil (cons-before-or-nil xs x)))
           ;; If we found a predecessor then we just have to modify its cdr to
           ;; skip the cons having x as car.  If we didn't find it there is
           ;; nothing to remove.
           (if (non-null? predecessor-or-nil) ;; We don't have when yet.
               ;; Notice that cddr is safe here, as predecessor-or-nil is the
               ;; predecessor of another element.
               (set-cdr! predecessor-or-nil (cddr predecessor-or-nil)))
           xs))))




;;;; all-different?.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; This is O(n^2).
(define-constant (all-different? xs)
  (cond ((null? xs)
         #t)
        ((list-has? (cdr xs) (car xs))
         #f)
        (else
         (all-different? (cdr xs)))))




;;;; High-level syntax: one-way conditionals.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-macro (when condition . body-forms)
  `(if ,condition
       (begin ,@body-forms)
       (begin)))

(define-macro (unless condition . body-forms)
  `(if ,condition
       (begin)
       (begin ,@body-forms)))




;;;; Variadic boolean connectives.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Return a generalized boolean which is the logical disjunction of the given
;;; clauses, evaluated left-to-right short-circuit; in case of a non-#f result
;;; which exact value is returned is unspecified.
;;; This definition is much laxer than the one common in other Lisp dialects;
;;; see the comment before lispy-or for a rationale.
(define-macro (or . clauses)
  (cond ((null? clauses)
         '#f)
        ((non-cons? clauses)
         (error '(or: non-list arguments)))
        ((null? (cdr clauses))
         (car clauses))
        (else
         `(if ,(car clauses)
              '#t
              (or ,@(cdr clauses))))))

;;; Return a generalized boolean which is the logical conjunction of the given
;;; clauses, evaluated left-to-right short-circuit; in case of a non-#f result
;;; which exact value is returned is unspecified.
;;; This specification is much laxer than the one common in other Lisp dialects,
;;; for symmetry with the or macro above, but the implementation in this case
;;; actually follows the Lisp tradition.  See the comment before lispy-or for a
;;; rationale.
(define-macro (and . clauses)
  (cond ((null? clauses)
         '#t)
        ((non-cons? clauses)
         (error '(and: non-list arguments)))
        ((null? (cdr clauses))
         (car clauses))
        (else
         `(if ,(car clauses)
              (and ,@(cdr clauses))
              '#f))))

;;; This is a more typical Lisp-style variadic or operator returning the first
;;; non-#f form result in case of a true conjunction.
;;; The problem is that the nested let blocks it expands to will be difficult to
;;; compile efficiently with the naïf code generator I have in mind for a stack
;;; machine.  It would work well if I did liveness analysis, and reused
;;; registers as soon as each variable died.
(define-macro (lispy-or . args)
  (cond ((null? args)
         '#f)
        ((null? (cdr args))
         (car args))
        (else
         (let* ((first-name (gensym)))
           `(let* ((,first-name ,(car args)))
              (if ,first-name
                  ,first-name
                  (lispy-or ,@(cdr args))))))))

;;; A variadic left-to-right short-circuit logical conjunction, returning the
;;; result of the last clause in case of a non-#f result according to the Lisp
;;; convention.
;;; This is provided just for symmetry, since JitterLisp's default and operator
;;; is already efficient, and differently from JitterLisp's or follows the Lisp
;;; convention.
(define lispy-and
  and)




;;;; High-level syntax: let.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Rewrite something like
;;;   (let ((a 1) (b 2) foo))
;;; into something like
;;;   (let* ((fresh-1 1) (fresh-2 2) (a fresh-1) (b fresh-2)) foo)
;;; .  The redundant bindings will be optimized away by AST rewriting.
(define-macro (let bindings . body-forms)
  (unless (all-different? (map car bindings))
    (error `(non-distinct let-bound variables in ,bindings)))
  (let* ((fresh-variables (map (lambda (irrelevant) (gensym))
                               bindings))
         (fresh-variable-bindings (zip fresh-variables
                                       (map cdr bindings)))
         (user-variable-bindings (zip (map car bindings)
                                      (map singleton fresh-variables))))
    `(let* ,(append-procedure fresh-variable-bindings
                              user-variable-bindings)
       ,@body-forms)))




;;;; High-level syntax: letrec.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-macro (letrec bindings . body-forms)
  (unless (all-different? (map car bindings))
    (error `(non-distinct letrec-bound variables in ,bindings)))
  `(let* ,(map (lambda (binding) `(,(car binding) (undefined)))
               bindings)
     ,@(map (lambda (binding) `(set! ,(car binding) ,@(cdr binding)))
            bindings)
     ,@body-forms))




;;;; High-level syntax: named let.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-macro (named-let loop-name bindings . body-forms)
  (unless (all-different? (map car bindings))
    (error `(non-distinct named-let-bound variables in ,bindings)))
  `(letrec ((,loop-name (lambda ,(map car bindings) ,@body-forms)))
     (,loop-name ,@(map (lambda (binding) `(begin ,@(cdr binding)))
                        bindings))))




;;;; High-level syntax: Scheme-style let, either named or not.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Redefine the let form to expand to either the previously defined let form or
;;; named-let, according to the shape of the first argument.  In order to do
;;; this we first have to save the previous let macro, to be reused in one of
;;; the two cases.
(define-constant previous-let
  let)
(define-macro (let first-argument . other-arguments)
  (if (symbol? first-argument)
      `(named-let ,first-argument ,@other-arguments)
      `(previous-let ,first-argument ,@other-arguments)))




;;;; Variadic procedure composition.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Variadic procedure composition.
(define-macro (compose . args)
  (cond ((null? args)
         'identity)
        ((null? (cdr args))
         (car args))
        (else
         `(compose-procedure ,(car args)
                             (compose ,@(cdr args))))))

;;; Sometimes it is convenient to write variadically composed procedures
;;; in the order they are executed.  This is equivalent to a use of the
;;; compose macro with its arguments in the opposite order, except that
;;; the arguments of this macro are still evaluated left-to-right, in
;;; the order they are written in the macro use.
(define-macro (compose-pipeline . args)
  (let ((procedure-names (map (lambda (useless) (gensym)) args)))
    `(let* ,(zip procedure-names
                 (map singleton args))
       (compose ,@(reverse procedure-names)))))




;;;; Variadic list or pseudo-list operations.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Given one or more arguments return their right-deep nested conses in order,
;;; ending with the last element.  Evaluate elements in the given order.
;;; Examples:
;;;   (improper-list 1)      ==> 1
;;;   (improper-list 1 2 3)  ==> (cons 1 (cons 2 3))
;;;   (improper-list 1 2 ()) ==> (cons 1 (cons 2 ()))
(define-macro (improper-list first-element . other-elements)
  (if (null? other-elements)
      first-element
      `(cons ,first-element
             (improper-list ,@other-elements))))

;;; This is the usual Scheme name.
(define cons*
  improper-list)

(define-macro (list . elements)
  `(improper-list ,@elements ()))

(define-macro (circular-list first-element . other-elements)
  (let* ((res-name (gensym))
         (cdr-name (gensym)))
    `(let* ((,res-name (cons ,first-element #f))
            (,cdr-name (improper-list ,@other-elements ,res-name)))
       (set-cdr! ,res-name ,cdr-name)
       ,res-name)))




;;;; High-level syntax: case form.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; FIXME: use let-macro
(define-macro (case-variable-matches? variable literals)
  (if (eq? literals 'else)
      '#t
      `(or ,@(map (lambda (a-literal)
                    `(eq? ,variable ,a-literal))
                  literals))))

;; FIXME: use let-macro
(define-macro (case-variable variable . clauses)
  (if (null? clauses)
      '(begin)
      (begin
        (unless (list? (car clauses))
          (error `(case: non-list clause ,(car clauses))))
        (unless (or (eq? (caar clauses) 'else)
                    (list? (caar clauses)))
          (error `(case: non-list non-else clause pattern ,(caar clauses))))
        `(if (case-variable-matches? ,variable ,(caar clauses))
             (begin ,@(cdar clauses))
             (case-variable ,variable ,@(cdr clauses))))))

(define-macro (case discriminand . clauses)
  (let ((discriminand-variable (gensym)))
    `(let ((,discriminand-variable ,discriminand))
       (case-variable ,discriminand-variable ,@clauses))))




;;;; High-level syntax: sequencing forms.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Evaluate forms in sequence, and return the result of the index-th one,
;;; 1-based.  For example (begin-from-first 2 . forms) returns the result
;;; of the second form.
(define-macro (begin-from-first index . forms)
  (unless (fixnum? index)
    (error `(begin-from-first: non-fixnum index ,index)))
  (unless (> index 0)
    (error `(begin-from-first: non-positive index ,index)))
  (unless (>= (length forms) index)
    (error `(begin-from-first: not enough forms in ,@forms)))
  (let ((result-variable (gensym))
        (first-forms (take forms index))
        (last-forms (drop forms index)))
    `(let ((,result-variable ,@first-forms))
       ,@last-forms
       ,result-variable)))

(define-macro (begin1 . forms)
  `(begin-from-first 1 ,@forms))
(define-macro (begin2 . forms)
  `(begin-from-first 2 ,@forms))
(define-macro (begin3 . forms)
  `(begin-from-first 3 ,@forms))
(define-macro (begin4 . forms)
  `(begin-from-first 4 ,@forms))

;;; Evaluate forms in sequence, and return the result of the index-th-to-last
;;; one, 1-based.  For example (begin-from-last 2 . forms) returns the result
;;; of the second-to-last form.  An index of 1 yields a macro functionally
;;; equivalent to begin .
(define-macro (begin-from-last index . forms)
  (unless (fixnum? index)
    (error `(begin-from-last: non-fixnum index ,index)))
  (unless (> index 0)
    (error `(begin-from-last: non-positive index ,index)))
  (unless (>= (length forms) index)
    (error `(begin-from-last: not enough forms in ,@forms)))
  (let* ((result-variable (gensym))
         (first-form-no (primordial-- (length forms) index))
         (first-forms (take forms first-form-no))
         (interesting-and-last-forms (drop forms first-form-no))
         (interesting-form (car interesting-and-last-forms))
         (last-forms (cdr interesting-and-last-forms)))
    `(begin
       ,@first-forms
       (let ((,result-variable ,interesting-form))
         ,@last-forms
         ,result-variable))))

(define-macro (begin-1 . forms)
  `(begin-from-last 1 ,@forms))
(define-macro (begin-2 . forms)
  `(begin-from-last 2 ,@forms))
(define-macro (begin-3 . forms)
  `(begin-from-last 3 ,@forms))
(define-macro (begin-4 . forms)
  `(begin-from-last 4 ,@forms))




;;;; High-level syntax: looping forms.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-macro (dotimes (variable limit . result-forms) . body-forms)
  (let ((limit-name (gensym)))
    `(let* ((,limit-name ,limit)
            (,variable 0))
       (while (< ,variable ,limit-name)
         ,@body-forms
         (set! ,variable (1+ ,variable)))
       ,@result-forms)))

(define-macro (dotimesdown (variable times . result-forms) . body-forms)
  (let ((times-name (gensym)))
    `(let ((,variable ,times))
       (while (> ,variable 0)
         (set! ,variable (1- ,variable))
         ,@body-forms)
       ,@result-forms)))

(define-macro (dolist (variable list . result-forms) . body-forms)
  (let ((list-name (gensym)))
    `(let* ((,list-name ,list)
            ;; This is currently faster than binding ,variable inside the loop.
            (,variable (undefined)))
       (while (non-null? ,list-name)
         (set! ,variable (car ,list-name))
         ,@body-forms
         (set! ,list-name (cdr ,list-name)))
       ,@result-forms)))

;; An alternative version of dolist , with the same semantics.  This definition
;; is likely more natural and should generate better compiled code, but will be
;; worse on the current AST interpreter where let requires heap allocation.
(define-macro (dolist-alt (variable list . result-forms) . body-forms)
  (let ((list-name (gensym)))
    `(let* ((,list-name ,list))
       (while (non-null? ,list-name)
         (let* ((,variable (car ,list-name)))
           ,@body-forms
           (set! ,list-name (cdr ,list-name))))
       ,@result-forms)))

(define-macro (do bindings (end-condition . result-forms) . body-forms)
  `(let (,@(map (lambda (binding)
                  `(,(car binding) ,(cadr binding)))
               bindings))
     (while (not ,end-condition)
       ,@body-forms
       ,@(flatten (map (lambda (binding)
                         (if (null? (cddr binding))
                             '()
                             `((set! ,(car binding) ,@(cddr binding)))))
                       bindings)))
     ,@result-forms))




;;;; Macro-level iteration.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Expand to an expression evaluating the given operator applied to each of the
;;; given operands, in sequence.  The expression is meant to have side effects,
;;; for example by applying definition forms; its final result is #<nothing>.
(define-macro (map-syntactically operator . operands)
  (if (null? operands)
      '(begin)
      `(begin
         (,operator ,(car operands))
         (map-syntactically ,operator ,@(cdr operands)))))

;;; Expand to an expression evaluating the given operators applied the one
;;; operand (given as the last argument), in sequence.  The expression is meant
;;; to have side effects, for example by applying definition, optimization or
;;; disassembly forms; its final result is #<nothing>.
;;; "pam" is "map" reversed, but I didn't use a "-reverse" suffix in the name
;;; as that suggests that the list is reversed..
(define-macro (pam-syntactically . operators-and-operand)
  (let ((operators (all-but-last operators-and-operand))
        (operand (last operators-and-operand)))
    `(begin
       ,@(map (lambda (operator) `(,operator ,operand))
              operators))))




;;;; Variadic operator procedures.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; It's quite convenient to define the machinery for nesting variadic operators
;;; as left- or right-deep s-expressions thru procedures, and then to wrap
;;; procedures into macros.  Defining macro-defining macros is more involved,
;;; for no gain.

;;; Return an s-expression encoding nested two-argument rator calls, using rands
;;; as the operands to be evaluated in order.  The neutral rand is only used if
;;; there are zero rands.  Nest on the neft.
;;; Examples:
;;;   (variadic-left-deep '+ 0 '(1 2 3)) ==> (+ (+ 1 2) 3)
;;;   (variadic-left-deep '+ 0 '(1))     ==> 1
;;;   (variadic-left-deep '+ 0 '())      ==> 0
(define-constant (variadic-left-deep rator neutral rands)
  (cond ((null? rands)
         neutral)
        ((null? (cdr rands))
         (car rands))
        (else
         (let* ((last-rand (last rands))
                (all-but-last-rands (all-but-last rands)))
           `(,rator ,(variadic-left-deep rator neutral
                                         all-but-last-rands)
                    ,last-rand)))))

;;; Like variadic-left-deep, but nesting on the right.
;;; Examples:
;;;   (variadic-right-deep '+ 0 '(1 2 3)) ==> (+ 1 (+ 2 3))
;;;   (variadic-right-deep '+ 0 '(1))     ==> 1
;;;   (variadic-right-deep '+ 0 '())      ==> 0
(define-constant (variadic-right-deep rator neutral rands)
  (cond ((null? rands)
         neutral)
        ((null? (cdr rands))
         (car rands))
        (else
         `(,rator ,(car rands)
                  ,(variadic-right-deep rator neutral (cdr rands))))))

;;; Define operator as a variadic macro, composing the original-name rator
;;; (itself a procedure, variable name or macro name) with the given
;;; netural element.  Nest on the left.
(define-macro (define-left-nested-variadic-extension operator original-name
                neutral)
  (let ((operands-name (gensym)))
    `(define-macro (,operator . ,operands-name)
       (variadic-left-deep ',original-name ',neutral ,operands-name))))

;;; Like define-left-nested-variadic-extension, but nest on the right.
(define-macro (define-right-nested-variadic-extension operator original-name
                neutral)
  (let ((operands-name (gensym)))
    `(define-macro (,operator . ,operands-name)
       (variadic-right-deep ',original-name ',neutral ,operands-name))))

;;; Define a variadic operator thru define-left-nested-variadic-extension or
;;; define-right-nested-variadic-extension ; it makes no difference which way
;;; we nest for an associative operators, so use whichever one happens to be
;;; more efficient in the current implementation.
(define-macro (define-associative-variadic-extension operator
                original-name neutral)
  ;; With a strict left-to-right evaluation order nesting on the left yields
  ;; better stack code.
  `(define-left-nested-variadic-extension ,operator ,original-name ,neutral))




;;;; Variadic arithmetic.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-associative-variadic-extension +
  primordial-+ 0)
(define-associative-variadic-extension *
  primordial-* 1)

(define-macro (- . operands)
  (cond ((null? operands)
         (error '(-: no arguments)))
        ((null? (cdr operands))
         `(negate ,@operands))
        (else
         `(primordial-- ,(car operands)
                        (+ ,@(cdr operands))))))

(define-macro (/ . operands)
  (cond ((null? operands)
         (error '(/: no arguments)))
        ((null? (cdr operands))
         `(primordial-/ 1 ,@operands))
        (else
         `(primordial-/ ,(car operands)
                        (* ,@(cdr operands))))))


;;;; Squaring.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (square n)
  (* n n))




;;;; Multiplication by additions (for fun).
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (*-nonnegative-b-by-sums-non-tail-recursive a b)
  (cond ((zero? b)
         0)
        ((odd? b)
         (+ a (*-nonnegative-b-by-sums-non-tail-recursive (2* a)
                                                          (2quotient b))))
        (else
         (*-nonnegative-b-by-sums-non-tail-recursive (2* a)
                                                     (2quotient b)))))
(define-constant (*-by-sums-non-tail-recursive a b)
  (if (< b 0)
      (- (*-nonnegative-b-by-sums-non-tail-recursive a (- b)))
      (*-nonnegative-b-by-sums-non-tail-recursive a b)))

(define-constant (*-nonnegative-b-by-sums-iterative a b)
  (let ((res 0))
    (while (not (zero? b))
      (when (odd? b)
        (set! res (+ res a)))
      (set! a (2* a))
      (set! b (2quotient b)))
    res))
(define-constant (*-by-sums-iterative a b)
  (if (< b 0)
      (- (*-nonnegative-b-by-sums-iterative a (- b)))
      (*-nonnegative-b-by-sums-iterative a b)))

(define-constant (*-by-sums-procedure a b)
  (*-by-sums-iterative a b))

(define-left-nested-variadic-extension *-by-sums
  ;; This is nested on the left, so that the second argument doesn't become
  ;; bigger and bigger in nested calls.
  *-by-sums-procedure 1)




;;;; Exponentiation.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (**-procedure-non-tail-recursive b e)
  (cond ((zero? e)
         1)
        ((even? e)
         (square (**-procedure-non-tail-recursive b (quotient e 2))))
        (else
         (* b (square (**-procedure-non-tail-recursive b (quotient e 2)))))))

(define-constant (**-procedure-iterative b e)
  (let ((res 1))
    (while (not (zero? e))
      (when (odd? e)
        (set! res (* res b))
        (set! e (- e 1)))
      (set! e (quotient e 2))
      (set! b (square b)))
    res))

(define-constant **-procedure
  **-procedure-iterative)

(define-right-nested-variadic-extension **-non-tail-recursive
  **-procedure-non-tail-recursive 1)
(define-right-nested-variadic-extension **-iterative
  **-procedure-iterative 1)

(define **
  **-iterative)




;;;; Making non-variadic lambdas from possibly variadic operators.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; This is a convenient way to syntactically generate a lambda from a macro
;;; name.
;;; Example:
;;;   (lambda-wrapper + 2)
;;;   expands to something equivalent to
;;;   (lambda (a b) (+ a b)) .
(define-macro (lambda-wrapper rator arity)
  (let ((formals (map (lambda (_) (gensym))
                      (iota arity))))
    `(lambda ,formals
       (,rator ,@formals))))




;;;; Variadic list operations.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Define variadic versions of append and append! , now finally with the
;;; appropriate names meant for the user.
(define-right-nested-variadic-extension append
  append-procedure ())
(define-right-nested-variadic-extension append!
  append!-procedure ())




;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; Sets as lists.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Sets implemented as unordered lists without duplicates, elements compared
;;; with eq? .

(define-constant set-empty
  ())

(define-constant (set-empty? xs)
  (null? xs))

(define-constant (set-has? xs x)
  (list-has? xs x))

(define-constant (set-without-helper xs x reversed-left-part)
  (cond ((null? xs)
         reversed-left-part)
        ((eq? (car xs) x)
         (append! reversed-left-part (cdr xs)))
        (else
         (set-without-helper (cdr xs) x (cons (car xs) reversed-left-part)))))
(define-constant (set-without xs x)
  (set-without-helper xs x ()))

(define-constant (set-with xs x)
  (cons x (set-without xs x)))

(define-constant (set-singleton x)
  (singleton x))

(define-constant (set-unite-procedure xs ys)
  (if (null? ys)
      xs
      (set-unite-procedure (set-with xs (car ys)) (cdr ys))))

(define-constant (set-subtract-procedure xs ys)
  (if (null? ys)
      xs
      (set-subtract-procedure (set-without xs (car ys)) (cdr ys))))

(define-constant (set-intersect-helper xs ys acc)
  (if (null? ys)
      acc
      (let* ((car-ys (car ys))
             (new-acc (if (set-has? xs car-ys)
                          (cons car-ys acc)
                          acc)))
        (set-intersect-helper xs
                              (cdr ys)
                              new-acc))))
(define-constant (set-intersect-procedure xs ys)
  (set-intersect-helper xs ys ()))

;;; Define variadic extensions for the union, intersection and subtraction
;;; operations.
(define-associative-variadic-extension set-unite
  set-unite-procedure set-empty)
(define-associative-variadic-extension set-intersect
  set-intersect-procedure set-empty)
(define-macro (set-subtract first-set . other-sets)
  `(set-subtract-procedure ,first-set (set-unite ,@other-sets)))

(define-constant (list->set list)
  ;; This relies on set-unite-procedure recurring on its second argument.
  (set-unite-procedure set-empty list))

;;; Return a fresh set-as-list containing the given elements, each to be
;;; evaluated left-to-right.  Duplicates are removed.
(define-macro (set . elements)
  (if (null? elements)
      ()
      ;; This is slightly complicated by the need to keep the evaluation order
      ;; intuitive: macro arguments are to be evaluated left-to-right.
      (let ((element-name (gensym))
            (subset-name (gensym)))
        ;; Using let* rather than let here is just an optimization.
        `(let* ((,element-name ,(car elements))
                (,subset-name (set ,@(cdr elements))))
           (set-with ,subset-name ,element-name)))))




;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; Tentative features, or experimentation just for fun.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;; Streams.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant stream-empty
  '(#t . ()))

(define-constant (stream-ready? s)
  (car s))

(define-constant (stream-force! s)
  (unless (stream-ready? s)
    (set-cdr! s ((cdr s)))
    (set-car! s #t))
  (cdr s))

(define-constant (stream-null? s)
  (null? (stream-force! s)))
(define-constant (stream-non-null? s)
  (non-null? (stream-force! s)))
(define-constant (stream-car s)
  (car (stream-force! s)))
(define-constant (stream-cdr s)
  (cdr (stream-force! s)))
(define-constant (stream-set-car! s new-car)
  (set-car! (stream-force! s) new-car))
(define-constant (stream-set-cdr! s new-cdr)
  (set-cdr! (stream-force! s) new-cdr))

(define-macro (stream-cons x s)
  `(cons #f
         (lambda ()
           (cons ,x ,s))))

(define-macro (stream-delay stream-expression)
  `(cons #f
         (lambda ()
           (stream-force! ,stream-expression))))

(define-constant (stream-forever-1 x)
  (letrec ((res (stream-delay (stream-cons x res))))
    res))

(define-constant (stream-ones)
  (stream-forever-1 1))

(define-constant (stream-from from)
  (stream-cons from (stream-from (1+ from))))
(define-constant (stream-naturals)
  (stream-from 0))

(define-constant (stream-walk-elements procedure s)
  (while (stream-non-null? s)
    (procedure (stream-car s))
    (set! s (stream-cdr s))))

(define-constant (stream-print-elements s)
  (stream-walk-elements (lambda (x)
                          (display x)
                          (newline))
                        s))

(define-constant (stream-touch-elements s)
  (stream-walk-elements (lambda (x))
                        s))

(define-constant (stream-range a b)
  (if (> a b)
      stream-empty
      (stream-cons a (stream-range (1+ a) b))))

(define-constant (stream-append s1 s2)
  (stream-delay
    (if (stream-null? s1)
        s2
        (stream-cons (stream-car s1)
                     (stream-append (stream-cdr s1) s2)))))

(define-constant (stream-forever-stream s)
  (letrec ((res (stream-delay (stream-append s res))))
    res))

(define-constant (stream-filter p? s)
  (stream-delay
    (cond ((stream-null? s)
           stream-empty)
          ((p? (stream-car s))
           (stream-cons (stream-car s)
                        (stream-filter p? (stream-cdr s))))
          (else
           (stream-filter p? (stream-cdr s))))))

(define-constant (stream-map f s)
  (stream-delay
    (if (stream-null? s)
        stream-empty
        (stream-cons (f (stream-car s))
                     (stream-map f (stream-cdr s))))))

(define-constant (stream-take s n)
  (stream-delay
    (cond ((zero? n)
           stream-empty)
          ((stream-null? s)
           stream-empty)
          (else
           (stream-cons (stream-car s)
                        (stream-take (stream-cdr s) (1- n)))))))

(define-constant (stream-drop s n)
  (stream-delay
    (cond ((zero? n)
           s)
          ((stream-null? s)
           stream-empty)
          (else
           (stream-drop (stream-cdr s) (1- n))))))

(define-constant (stream-fold-left f x xs)
  (if (stream-null? xs)
      x
      (stream-fold-left f
                        (f x (stream-car xs))
                        (stream-cdr xs))))




;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; AST optimization.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;; Compute the set of variables occurring free in an AST.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Return a set-as-list of the variables occurring free in the given AST.
(define-constant (ast-free ast)
  (cond ((ast-literal? ast)
         set-empty)
        ((ast-variable? ast)
         (set-singleton (ast-variable-name ast)))
        ((ast-define? ast)
         ;; The defined variable is global, and doesn't enter the picture at
         ;; all.
         (ast-free (ast-define-body ast)))
        ((ast-if? ast)
         (set-unite (ast-free (ast-if-condition ast))
                    (ast-free (ast-if-then ast))
                    (ast-free (ast-if-else ast))))
        ((ast-set!? ast)
         ;; An assigned variable *is* a reference in the sense of this
         ;; procedure, differently from a defined global.  Of course the
         ;; modified binding may be global, but the variable is free.
         (set-with (ast-free (ast-set!-body ast))
                   (ast-set!-name ast)))
        ((ast-while? ast)
         (set-unite (ast-free (ast-while-guard ast))
                    (ast-free (ast-while-body ast))))
        ((ast-primitive? ast)
         (ast-free-list (ast-primitive-operands ast)))
        ((ast-call? ast)
         (set-unite (ast-free (ast-call-operator ast))
                    (ast-free-list (ast-call-operands ast))))
        ((ast-lambda? ast)
         (set-subtract (ast-free (ast-lambda-body ast))
                       (ast-lambda-formals ast)))
        ((ast-let? ast)
         (set-unite (ast-free (ast-let-bound-form ast))
                    (set-without (ast-free (ast-let-body ast))
                                 (ast-let-bound-name ast))))
        ((ast-sequence? ast)
         (set-unite (ast-free (ast-sequence-first ast))
                    (ast-free (ast-sequence-second ast))))))

;;; Return a set-as-list of the free variables in the given list of ASTs.
(define-constant (ast-free-list asts)
  (if (null? asts)
      set-empty
      (set-unite (ast-free (car asts))
                 (ast-free-list (cdr asts)))))




;;;; Check whether a given variable occurs free in an AST.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Return non-#f iff the given variable occurs free in the given AST.
(define-constant (ast-has-free? ast x)
  (cond ((ast-literal? ast)
         #f)
        ((ast-variable? ast)
         (eq? (ast-variable-name ast) x))
        ((ast-define? ast)
         ;; The defined variable is global, and doesn't enter the picture.
         (ast-has-free? (ast-define-body ast) x))
        ((ast-if? ast)
         (or (ast-has-free? (ast-if-condition ast) x)
             (ast-has-free? (ast-if-then ast) x)
             (ast-has-free? (ast-if-else ast) x)))
        ((ast-set!? ast)
         ;; An assigned variable *is* a reference in the sense of this
         ;; procedure, differently from a defined global.  Of course the
         ;; modified binding may be global, but the variable is free.
         (or (eq? (ast-set!-name ast) x)
             (ast-has-free? (ast-set!-body ast) x)))
        ((ast-while? ast)
         (or (ast-has-free? (ast-while-guard ast) x)
             (ast-has-free? (ast-while-body ast) x)))
        ((ast-primitive? ast)
         (ast-has-free?-list (ast-primitive-operands ast) x))
        ((ast-call? ast)
         (or (ast-has-free? (ast-call-operator ast) x)
             (ast-has-free?-list (ast-call-operands ast) x)))
        ((ast-lambda? ast)
         (if (set-has? (ast-lambda-formals ast) x)
             #f
             (ast-has-free? (ast-lambda-body ast) x)))
        ((ast-let? ast)
         (if (eq? (ast-let-bound-name ast) x)
             (ast-has-free? (ast-let-bound-form ast) x)
             (or (ast-has-free? (ast-let-bound-form ast) x)
                 (ast-has-free? (ast-let-body ast) x))))
        ((ast-sequence? ast)
         (or (ast-has-free? (ast-sequence-first ast) x)
             (ast-has-free? (ast-sequence-second ast) x)))))

;;; An extension of ast-has-free? to a list of ASTs.
(define-constant (ast-has-free?-list asts x)
  (and (non-null? asts)
       (or (ast-has-free? (car asts) x)
           (ast-has-free?-list (cdr asts) x))))




;;;; Tentative: assigned variables in an AST.  [FIXME: remove unless I use this]
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; A set! form counts as an assignment; a define form doesn't, as a
;;; define'd variable in JitterLisp is always a global, and globals
;;; can't be renamed.

;;; Return a set-as-list of the free variables being set! in the given AST.
(define-constant (ast-assigned ast)
  (cond ((ast-literal? ast)
         set-empty)
        ((ast-variable? ast)
         set-empty)
        ((ast-define? ast)
         ;; The defined variable is global: see the comment above.
         (ast-assigned (ast-define-body ast)))
        ((ast-if? ast)
         (set-unite (ast-assigned (ast-if-condition ast))
                    (ast-assigned (ast-if-then ast))
                    (ast-assigned (ast-if-else ast))))
        ((ast-set!? ast)
         (set-with (ast-assigned (ast-set!-body ast))
                   (ast-set!-name ast)))
        ((ast-while? ast)
         (set-unite (ast-assigned (ast-while-guard ast))
                    (ast-assigned (ast-while-body ast))))
        ((ast-primitive? ast)
         (ast-assigned-list (ast-primitive-operands ast)))
        ((ast-call? ast)
         (set-unite (ast-assigned (ast-call-operator ast))
                    (ast-assigned-list (ast-call-operands ast))))
        ((ast-lambda? ast)
         (set-subtract (ast-assigned (ast-lambda-body ast))
                       (ast-lambda-formals ast)))
        ((ast-let? ast)
         (set-unite (ast-assigned (ast-let-bound-form ast))
                    (set-without (ast-assigned (ast-let-body ast))
                                 (ast-let-bound-name ast))))
        ((ast-sequence? ast)
         (set-unite (ast-assigned (ast-sequence-first ast))
                    (ast-assigned (ast-sequence-second ast))))))

;;; Return a set-as-list of the free variables being set! in the given list of
;;; ASTs.
(define-constant (ast-assigned-list asts)
  (if (null? asts)
      set-empty
      (set-unite (ast-assigned (car asts))
                 (ast-assigned-list (cdr asts)))))




;;;; Assigned variables in an AST.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; A set! form counts as an assignment; a define form doesn't, as a
;;; define'd variable in JitterLisp is always a global, and globals
;;; can't be renamed by alpha-conversion.

;;; Return non-#f iff any free occurrence of the given variable is set! in the
;;; given AST.
(define-constant (ast-has-assigned? ast x)
  (cond ((ast-literal? ast)
         #f)
        ((ast-variable? ast)
         #f)
        ((ast-define? ast)
         ;; The defined variable is global: see the comment above.
         (ast-has-assigned? (ast-define-body ast) x))
        ((ast-if? ast)
         (or (ast-has-assigned? (ast-if-condition ast) x)
             (ast-has-assigned? (ast-if-then ast) x)
             (ast-has-assigned? (ast-if-else ast) x)))
        ((ast-set!? ast)
         (or (eq? (ast-set!-name ast) x)
             (ast-has-assigned? (ast-set!-body ast) x)))
        ((ast-while? ast)
         (or (ast-has-assigned? (ast-while-guard ast) x)
             (ast-has-assigned? (ast-while-body ast) x)))
        ((ast-primitive? ast)
         (ast-has-assigned?-list (ast-primitive-operands ast) x))
        ((ast-call? ast)
         (or (ast-has-assigned? (ast-call-operator ast) x)
             (ast-has-assigned?-list (ast-call-operands ast) x)))
        ((ast-lambda? ast)
         (if (set-has? (ast-lambda-formals ast) x)
             #f
             (ast-has-assigned? (ast-lambda-body ast) x)))
        ((ast-let? ast)
         (if (eq? (ast-let-bound-name ast) x)
             (ast-has-assigned? (ast-let-bound-form ast) x)
             (or (ast-has-assigned? (ast-let-bound-form ast) x)
                 (ast-has-assigned? (ast-let-body ast) x))))
        ((ast-sequence? ast)
         (or (ast-has-assigned? (ast-sequence-first ast) x)
             (ast-has-assigned? (ast-sequence-second ast) x)))))

;;; An extension of ast-has-assigned? to a list of ASTs.
(define-constant (ast-has-assigned?-list asts x)
  (and (non-null? asts)
       (or (ast-has-assigned? (car asts) x)
           (ast-has-assigned?-list (cdr asts) x))))




;;;; Non-locally used variables in an AST.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Check whether a variable is accessed non-locally, thru a lambda occurring
;;; within the given AST.  For the purposes of this definition nested lets do
;;; not count: only lambdas introduce the kind of non-locality we care about.
;;; Variables shadowed in the lambda don't count eihter (but there shouldn't
;;; be any, as this is supposed to be used on an already alpha-converted AST).

;;; Return non-#f iff any free occurrence of the given variable occurs free in a
;;; lambda syntactically contained within the given AST.
(define-constant (ast-nonlocally-uses? ast x)
  (cond ((ast-literal? ast)
         #f)
        ((ast-variable? ast)
         #f)
        ((ast-define? ast)
         ;; The defined variable is global, and therefore irrelevant here.
         (ast-nonlocally-uses? (ast-define-body ast) x))
        ((ast-if? ast)
         (or (ast-nonlocally-uses? (ast-if-condition ast) x)
             (ast-nonlocally-uses? (ast-if-then ast) x)
             (ast-nonlocally-uses? (ast-if-else ast) x)))
        ((ast-set!? ast)
         ;; By itself this assignment, even if it were on x, is irrelevant
         ;; because it's not within a lambda.  However the set! body might
         ;; contain a lambda using x.
         (ast-nonlocally-uses? (ast-set!-body ast) x))
        ((ast-while? ast)
         (or (ast-nonlocally-uses? (ast-while-guard ast) x)
             (ast-nonlocally-uses? (ast-while-body ast) x)))
        ((ast-primitive? ast)
         (ast-nonlocally-uses?-list (ast-primitive-operands ast) x))
        ((ast-call? ast)
         (or (ast-nonlocally-uses? (ast-call-operator ast) x)
             (ast-nonlocally-uses?-list (ast-call-operands ast) x)))
        ((ast-lambda? ast)
         ;; This is the interesting case.
         (if (set-has? (ast-lambda-formals ast) x)
             ;; Shadowing: any occurrence of x within the lambda doesn't refer
             ;; the same x.
             #f
             ;; The lambda formals don't shadow x.  Check whether x occurs free
             ;; in the lambda body, which includes any sub-lambda.  Checking for
             ;; only free occurrences is essential: we don't want the analysis
             ;; to be fooled by deeper shadowing within the lambda.
             (ast-has-free? (ast-lambda-body ast) x)))
        ((ast-let? ast)
         ;; If this let shadows x then we don't care about its body, but we do
         ;; care about the bound form, which might contain a lambda using x.
         ;; Otherwise we care about both the bound form and the body.
         (if (eq? (ast-let-bound-name ast) x)
             (ast-nonlocally-uses? (ast-let-bound-form ast) x)
             (or (ast-nonlocally-uses? (ast-let-bound-form ast) x)
                 (ast-nonlocally-uses? (ast-let-body ast) x))))
        ((ast-sequence? ast)
         (or (ast-nonlocally-uses? (ast-sequence-first ast) x)
             (ast-nonlocally-uses? (ast-sequence-second ast) x)))))

;;; An extension of ast-nonlocally-uses? to a list of ASTs.
(define-constant (ast-nonlocally-uses?-list asts x)
  (and (non-null? asts)
       (or (ast-nonlocally-uses? (car asts) x)
           (ast-nonlocally-uses?-list (cdr asts) x))))




;;;; Boxed variables in an AST.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; This check is used to implement what David A. Kranz called "Assignment
;;; Conversion" in his thesis about the Orbit compiler.

;;; Check if a given local variable is at the same time *used* nonlocally
;;; (assigned or not, it doesn't matter) and assigned (locally or not, it
;;; doesn't matter): that is the case where a variable needs to be boxed.  Here,
;;; like in the section above, non-locally means within a lambda contained,
;;; directly or not, within the given AST.
;;; We are looking at free occurrences of the variable, not at inner bindings
;;; shadowing outer variables with the same name.
;;;
;;; A let block doesn't count for these purposes, as by itself a let within the
;;; same procedure doesn't require boxing and I can keep a let-bound variable in
;;; a register without any indirections, as long as assignments to the variable
;;; don't need to be visible across lambdas.
;;;
;;; This is, of course, conservative: it would be possible to do better in some
;;; cases, for example when a variable is assigned only locally and *before* the
;;; lambda using it is reached.

;;; Return non-#f iff the given variable needs to be boxed in the given AST.
(define-constant (ast-requires-boxing-for? ast x)
  (and (ast-nonlocally-uses? ast x)
       (ast-has-assigned? ast x)))




;;;; Non-locally assigned variables in an AST.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; FIXME: no, this idea is not what I need.  I need to check if a given local
;;; is at the same time *used* nonlocally (assigned or not, it doesn't matter)
;;; and assigned (locally or not, it doesn't matter).  That is the case where
;;; a variable needs to be boxed.  Remove this section.

;;; Check whether a variable is assigned non-locally in a lambda occurring
;;; within the given AST.  For the purposes of this definition nested lets do
;;; not count: only lambdas introduce the kind of non-locality we care about.
;;;
;;; Rationale: a set! on a non-local variable forces us to access the variable
;;; locally thru a box, also pointed by any closure closing over the variable;
;;; if, instead, a variable is accessed non-locally without being assigned in
;;; the lambda, this indirection is not necessary.  The same way, we don't need
;;; the box if the variable is assigned *locally* (including within a let, but
;;; not within a lambda).
;;; Compiled code relies on this, and I don't want to introduce boxes when it's
;;; not needed.

;;; Return non-#f iff any free occurrence of the given variable is set! (as per
;;; ast-has-assigned?) in a lambda syntactically contained within the given AST.
(define-constant (ast-nonlocally-assigns? ast x)
  (cond ((ast-literal? ast)
         #f)
        ((ast-variable? ast)
         #f)
        ((ast-define? ast)
         ;; The defined variable is global, and therefore irrelevant here.
         (ast-nonlocally-assigns? (ast-define-body ast) x))
        ((ast-if? ast)
         (or (ast-nonlocally-assigns? (ast-if-condition ast) x)
             (ast-nonlocally-assigns? (ast-if-then ast) x)
             (ast-nonlocally-assigns? (ast-if-else ast) x)))
        ((ast-set!? ast)
         ;; By itself this assignment, even if it were on x, is irrelevant
         ;; because it's not within a lambda.  However the set! body might
         ;; contain a lambda.
         (ast-nonlocally-assigns? (ast-set!-body ast) x))
        ((ast-while? ast)
         (or (ast-nonlocally-assigns? (ast-while-guard ast) x)
             (ast-nonlocally-assigns? (ast-while-body ast) x)))
        ((ast-primitive? ast)
         (ast-nonlocally-assigns?-list (ast-primitive-operands ast) x))
        ((ast-call? ast)
         (or (ast-nonlocally-assigns? (ast-call-operator ast) x)
             (ast-nonlocally-assigns?-list (ast-call-operands ast) x)))
        ((ast-lambda? ast)
         ;; This is the interesting case: we search for assignment to *free*
         ;; occurrences of x within the lambda -- including within lambdas
         ;; nested within this one.
         ;; If the lambda shadows x, then our own variable is not visible
         ;; there and we know that it's not assigned non-locally.
         ;; If the lambda doesn't shadow x, then we look for any assigned
         ;; free occurrence within the lambda body.  Notice that the call
         ;; examining the body doesn't recur to this function, but uses
         ;; ast-has-assigned? since we have already crossed the one lambda
         ;; boundary we were searching for: from this point the level of
         ;; nesting of lambdas is no longer important.
         (if (set-has? (ast-lambda-formals ast) x)
             #f
             (ast-has-assigned? (ast-lambda-body ast) x)))
        ((ast-let? ast)
         ;; If this let shadows x then we don't care about its body, but we do
         ;; care about the bound form, which might contain a lambda assigning x.
         ;; Otherwise we care about both the bound form and the body.
         (if (eq? (ast-let-bound-name ast) x)
             (ast-nonlocally-assigns? (ast-let-bound-form ast) x)
             (or (ast-nonlocally-assigns? (ast-let-bound-form ast) x)
                 (ast-nonlocally-assigns? (ast-let-body ast) x))))
        ((ast-sequence? ast)
         (or (ast-nonlocally-assigns? (ast-sequence-first ast) x)
             (ast-nonlocally-assigns? (ast-sequence-second ast) x)))))

;;; An extension of ast-nonlocally-assigns? to a list of ASTs.
(define-constant (ast-nonlocally-assigns?-list asts x)
  (and (non-null? asts)
       (or (ast-nonlocally-assigns? (car asts) x)
           (ast-nonlocally-assigns?-list (cdr asts) x))))




;;;; AST equality.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Return non-#f iff two ASTs are syntactically equal.  This could of course be
;;; made more accurate, for example by renaming bound variables in a consistent
;;; way in the two ASTs.
;;; Assume that a and b are both non-cyclic.
(define-constant (ast-equal? a b)
  (cond ((ast-literal? a)
         (and (ast-literal? b)
              (eq? (ast-literal-value a) (ast-literal-value b))))
        ((ast-variable? a)
         (and (ast-variable? b)
              (eq? (ast-variable-name a) (ast-variable-name b))))
        ((ast-define? a)
         (and (ast-define? b)
              (eq? (ast-define-name a) (ast-define-name b))
              (ast-equal? (ast-define-body a) (ast-define-body b))))
        ((ast-if? a)
         (and (ast-if? b)
              (ast-equal? (ast-if-condition a) (ast-if-condition b))
              (ast-equal? (ast-if-then a) (ast-if-then b))
              (ast-equal? (ast-if-else a) (ast-if-else b))))
        ((ast-set!? a)
         (and (ast-set!? b)
              (eq? (ast-set!-name a) (ast-set!-name b))
              (ast-equal? (ast-set!-body a) (ast-set!-body b))))
        ((ast-while? a)
         (and (ast-while? b)
              (ast-equal? (ast-while-guard a) (ast-while-guard b))
              (ast-equal? (ast-while-body a) (ast-while-body b))))
        ((ast-primitive? a)
         (and (ast-primitive? b)
              (eq? (ast-primitive-operator a) (ast-primitive-operator b))
              (ast-equal?-list (ast-primitive-operands a) (ast-primitive-operands b))))
        ((ast-call? a)
         (and (ast-call? b)
              (ast-equal? (ast-call-operator a) (ast-call-operator b))
              (ast-equal?-list (ast-call-operands a) (ast-call-operands b))))
        ((ast-lambda? a)
         (and (ast-lambda? b)
              (eq?-list (ast-lambda-formals a) (ast-lambda-formals b))
              (ast-equal? (ast-lambda-body a) (ast-lambda-body b))))
        ((ast-let? a)
         (and (ast-let? b)
              (eq? (ast-let-bound-name a) (ast-let-bound-name b))
              (ast-equal? (ast-let-bound-form a) (ast-let-bound-form b))
              (ast-equal? (ast-let-body a) (ast-let-body b))))
        ((ast-sequence? a)
         (and (ast-sequence? b)
              (ast-equal? (ast-sequence-first a) (ast-sequence-first b))
              (ast-equal? (ast-sequence-second a) (ast-sequence-second b))))))

;;; An extension of ast-equal? to lists of ASTs.
;;; Return non-#f iff the two given AST lists have syntactically equal elements
;;; at the same positions.  Return #f if the lists have different lengths or if
;;; any two elements at the same position are different.
(define-constant (ast-equal?-list as bs)
  (cond ((null? as)
         ;; Both empty lists, or different lengths.
         (null? bs))
        ((null? bs)
         ;; Different lengths.
         #f)
        ((ast-equal? (car as) (car bs))
         (ast-equal?-list (cdr as) (cdr bs)))
        (else
         ;; Different first elements.
         #f)))

;;; A helper for ast-equal?.  Return #t iff the two given non-cyclic lists of
;;; eq?-comparable values (used with symbols) are equal.
(define-constant (eq?-list as bs)
  (cond ((null? as)
         ;; Both empty lists, or different lengths.
         (null? bs))
        ((null? bs)
         ;; Different lengths.
         #f)
        ((eq? (car as) (car bs))
         (eq?-list (cdr as) (cdr bs)))
        (else
         ;; Different first elements.
         #f)))




;;;; AST alpha-conversion.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Return an AST equal to the given one, except that every bound variable
;;; has been consistently renamed to a fresh identifier.
(define-constant (ast-alpha-convert ast)
  (ast-alpha-convert-with ast ()))

;;; A helper procedure for ast-alpha-convert, keeping track of which free
;;; variable variable is to be replaced with which new variable.  Recursive
;;; calls will build new alists extending the given one, with a fresh new
;;; variable associated to each inner bound variable.
(define-constant (ast-alpha-convert-with ast alist)
  (cond ((ast-literal? ast)
         ast)
        ((ast-variable? ast)
         (let ((pair (assq (ast-variable-name ast) alist)))
           (if pair
               (ast-variable (cdr pair))
               ast)))
        ((ast-define? ast)
         ;; Do not rename globally bound variables.
         (ast-define (ast-define-name ast)
                     (ast-alpha-convert-with (ast-define-body ast) alist)))
        ((ast-if? ast)
         (ast-if (ast-alpha-convert-with (ast-if-condition ast) alist)
                 (ast-alpha-convert-with (ast-if-then ast) alist)
                 (ast-alpha-convert-with (ast-if-else ast) alist)))
        ((ast-set!? ast)
         (let* ((old-name (ast-set!-name ast))
                (pair (assq old-name alist))
                (new-name (if pair (cdr pair) old-name)))
           (ast-set! new-name
                     (ast-alpha-convert-with (ast-set!-body ast) alist))))
        ((ast-while? ast)
         (ast-while (ast-alpha-convert-with (ast-while-guard ast) alist)
                    (ast-alpha-convert-with (ast-while-body ast) alist)))
        ((ast-primitive? ast)
         (ast-primitive (ast-primitive-operator ast)
                        (ast-alpha-convert-with-list
                            (ast-primitive-operands ast)
                            alist)))
        ((ast-call? ast)
         (ast-call (ast-alpha-convert-with (ast-call-operator ast) alist)
                   (ast-alpha-convert-with-list (ast-call-operands ast)
                                                alist)))
        ((ast-lambda? ast)
         (let* ((old-formals (ast-lambda-formals ast))
                (new-formals (map (lambda (useless) (gensym)) old-formals))
                (new-bindings (zip-reversed old-formals new-formals))
                (new-alist (append! new-bindings alist)))
           (ast-lambda new-formals
                       (ast-alpha-convert-with (ast-lambda-body ast)
                                               new-alist))))
        ((ast-let? ast)
         (let* ((old-bound-name (ast-let-bound-name ast))
                (new-bound-name (gensym))
                (new-alist (cons (cons old-bound-name new-bound-name) alist)))
           (ast-let new-bound-name
                    (ast-alpha-convert-with (ast-let-bound-form ast)
                                            alist) ;; Important: not new-alist
                    (ast-alpha-convert-with (ast-let-body ast)
                                            new-alist))))
        ((ast-sequence? ast)
         (ast-sequence (ast-alpha-convert-with (ast-sequence-first ast)
                                               alist)
                       (ast-alpha-convert-with (ast-sequence-second ast)
                                               alist)))))

;;; An extension of ast-alpha-convert-with to a list of ASTs: return the list
;;; of rewriten ASTs in order.
(define-constant (ast-alpha-convert-with-list asts alist)
  (map (lambda (ast) (ast-alpha-convert-with ast alist))
       asts))




;;;; Global constant folding.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Return a copy of the given AST where every free occurrence of current global
;;; constants in replaced with its global value as a literal.  The additional
;;; argument is a set-of-list of the currently bound variables; of course bound
;;; variables just happening to share a name with a global constant must not be
;;; replaced.
(define-constant (ast-global-fold ast bounds)
  (cond ((ast-literal? ast)
         ast)
        ((ast-variable? ast)
         (let ((name (ast-variable-name ast)))
           (if (and (not (set-has? bounds name))
                    (defined? name)
                    (constant? name))
               (ast-literal (symbol-global name))
               ast)))
        ((ast-define? ast)
         (ast-define (ast-define-name ast)
                     (ast-global-fold (ast-define-body ast) bounds)))
        ((ast-if? ast)
         (ast-if (ast-global-fold (ast-if-condition ast) bounds)
                 (ast-global-fold (ast-if-then ast) bounds)
                 (ast-global-fold (ast-if-else ast) bounds)))
        ((ast-set!? ast)
         (ast-set! (ast-set!-name ast)
                   (ast-global-fold (ast-set!-body ast) bounds)))
        ((ast-while? ast)
         (ast-while (ast-global-fold (ast-while-guard ast) bounds)
                    (ast-global-fold (ast-while-body ast) bounds)))
        ((ast-primitive? ast)
         (ast-primitive (ast-primitive-operator ast)
                        (ast-global-fold-list (ast-primitive-operands ast)
                                              bounds)))
        ((ast-call? ast)
         (ast-call (ast-global-fold (ast-call-operator ast) bounds)
                   (ast-global-fold-list (ast-call-operands ast)
                                         bounds)))
        ((ast-lambda? ast)
         (let ((formals (ast-lambda-formals ast)))
           (ast-lambda formals
                       (ast-global-fold (ast-lambda-body ast)
                                        (set-unite formals bounds)))))
        ((ast-let? ast)
         (let* ((bound-name (ast-let-bound-name ast))
                (old-bound-form (ast-let-bound-form ast))
                (new-bound-form (ast-global-fold old-bound-form bounds))
                (old-body (ast-let-body ast))
                (new-body (ast-global-fold old-body
                                           (set-with bounds bound-name))))
           (ast-let bound-name
                    new-bound-form
                    new-body)))
        ((ast-sequence? ast)
         (ast-sequence (ast-global-fold (ast-sequence-first ast) bounds)
                       (ast-global-fold (ast-sequence-second ast)
                                        bounds)))))

;;; An extension of ast-global-fold to a list of ASTs: return the list
;;; of rewriten ASTs in order.
(define-constant (ast-global-fold-list asts bounds)
  (map (lambda (ast) (ast-global-fold ast bounds))
       asts))




;;;; Effect analysis.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; FIXME: it would be useful to define a notion similar to this but weaker, not
;;; considering allocation an effect.  This would let me remove some useless
;;; allocations.

;;; Return non-#f iff the evaluation of the given may have effects, in an
;;; environment where with the given set-as-list of variables are bound.
;;;
;;; The analysis is, of course, partial: a #f result means that the AST is
;;; guaranteed not to have effects.  A non-#f result means that the AST
;;; evaluation *may* have effects, which is meant to direct optimizers to err on
;;; the safe side.
;;; An analysis more precise than this is certainly possible, but catching every
;;; possible case of effects would require solving the Halting Problem.
;;;
;;; Notice that accessing a variable which is not bound and is not a constant
;;; is considered an effectful operation: the variable may be undefined as a
;;; global at evaluation time, which would cause a runtime error -- a visible
;;; effect.
;;; Any other error, including type errors, is also an effect.
;;; Heap allocation is an effect (allocations cannot in general be merged
;;;   without changing the program semantics).
;;; Non-termination is also an effect.
(define-constant (ast-effectful? ast bounds)
  (cond ((ast-literal? ast)
         #f)
        ((ast-variable? ast)
         (cond ((set-has? bounds (ast-variable-name ast))
                ;; Reading a bound variable has no effects.
                #f)
               ((constant? (ast-variable-name ast))
                ;; Reading a global constant has no effects.
                #f)
               (else
                ;; Reading a global non-constant may fail, which counts as an
                ;; effect.
                #t)))
        ((ast-define? ast)
         ;; Global definitions can have effects.
         #t)
        ((ast-if? ast)
         (or (ast-effectful? (ast-if-condition ast) bounds)
             (ast-effectful? (ast-if-then ast) bounds)
             (ast-effectful? (ast-if-else ast) bounds)))
        ((ast-set!? ast)
         ;; Assignments can have effects, and usually do.
         #t)
        ((ast-while? ast)
         ;; A while loop can have effects even if its guard and body are both
         ;; non-effectful, since non-termination is an effect.
         (let ((guard (ast-while-guard ast)))
           ;; The only case where we bother returning a more precise result is a
           ;; while loop with the constant #f as guard.
           (if (and (ast-literal? guard)
                    (not (ast-literal-value guard)))
               #f
               ;; The guard is not #f.  Consider the loop effectful.
               #t)))
        ((ast-primitive? ast)
         ;; Some primitives are known to be non-effectful, but all of their
         ;; arguments must be non-effectful as well for the entire AST to be
         ;; non-effectful.
         ;; It would be possible to be more precise and check for types in
         ;; some cases, where arguments are known; this would be useful for
         ;; arithmetic.
         (or (primitive-effectful? (ast-primitive-operator ast))
             (ast-effectful?-list (ast-primitive-operands ast)
                                  bounds)))
        ((ast-call? ast)
         ;; It's not trivial to return a more precise result here without
         ;; visiting a call graph of constant callees to be recursively
         ;; analyzed, keeping termination into account.  Right now I consider
         ;; any call effectful.
         #t)
        ((ast-lambda? ast)
         ;; Making a new closure is effectful, as an allocation operation.
         #t)
        ((ast-let? ast)
         (or (ast-effectful? (ast-let-bound-form ast)
                             bounds)
             (ast-effectful? (ast-let-body ast)
                             (set-with bounds
                                       (ast-let-bound-name ast)))))
        ((ast-sequence? ast)
         (or (ast-effectful? (ast-sequence-first ast) bounds)
             (ast-effectful? (ast-sequence-second ast) bounds)))))

;;; An extension of ast-effectful? to a list of ASTs: return #f iff
;;; *all* the ASTs in the list are known to be non-effectful.
(define-constant (ast-effectful?-list asts bounds)
  (cond ((null? asts)
         #f)
        ((ast-effectful? (car asts) bounds)
         #t)
        (else
         (ast-effectful?-list (cdr asts) bounds))))

;;; A set-as-list of non-effectful primitives.  FIXME: add a
;;; primitive-effectful? primitive and use it, instead of this.
;;;
;;; Notice that any primitives having type requirements on its arguments
;;; may fail, and is therefore effectful.  A primitive always returning
;;; a result given any argument is non-effectful.
;;; Type checking (for example primitive-null?) is therefore non-effectful,
;;; but case checking (for example primitive-ast-variable? or
;;; primitive-zero? , which fails on non-numbers) is effectful.
;;;
;;; Allocation primitives are effectful: primitive uses cannot be eliminated
;;; without changing the program semantics, in the presence of side effects.
;;; This makes primitive-cons and primitive-gensym effectful.
;;;
;;; Nullary primitives depending on global modifiable state, such as
;;; primitive-interned-symbols are effectful: their result may be different at a
;;; later time when the AST is actually executed.
(define-constant non-effectful-primitives
  (set ;; Type-checking primitives may return #f, but never fail.
       primitive-fixnum?
       primitive-character?
       primitive-null?
       primitive-non-null?
       primitive-eof?
       primitive-boolean?
       primitive-nothing?
       primitive-undefined?
       primitive-symbol?
       primitive-non-symbol?
       primitive-cons?
       primitive-non-cons?
       primitive-box?
       primitive-closure?
       primitive-primitive?
       primitive-ast?
       primitive-macro?
       primitive-vector?
       ;; Equality comparisons never fail (but maginitude comparisons may).
       primitive-eq?
       primitive-not-eq?
       ;; Logical negation and canonicalization accept a generalized boolean,
       ;; and therefore never fail.
       primitive-not
       primitive-boolean-canonicalize))

;;; Any primitive not in the set-as-list above is considered to be effectful.
(define-constant (primitive-effectful? primitive)
  (not (set-has? non-effectful-primitives primitive)))




;;;; Instantiation.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Return a copy of the given AST where every free occurrence of the given
;;; variable is replaced with the given AST.  Error out if a free occurrence
;;; of the variable is assigned, in which case instantiation would be invalid.
;;; Of course there are other cases in which instantiation would yield an AST
;;; not equivalent to the original: this is not checked for.
(define-constant (ast-instantiate ast x r)
  (cond ((ast-literal? ast)
         ast)
        ((ast-variable? ast)
         (if (eq? (ast-variable-name ast) x)
             r
             ast))
        ((ast-define? ast)
         ;; Do not rename globally bound variables.
         (ast-define (ast-define-name ast)
                     (ast-instantiate (ast-define-body ast) x r)))
        ((ast-if? ast)
         (ast-if (ast-instantiate (ast-if-condition ast) x r)
                 (ast-instantiate (ast-if-then ast) x r)
                 (ast-instantiate (ast-if-else ast) x r)))
        ((ast-set!? ast)
         (let ((name (ast-set!-name ast)))
           (when (eq? name x)
             (error `(instantiating assigned variable ,x in ,ast)))
           (ast-set! name
                     (ast-instantiate (ast-set!-body ast) x r))))
        ((ast-while? ast)
         (ast-while (ast-instantiate (ast-while-guard ast) x r)
                    (ast-instantiate (ast-while-body ast) x r)))
        ((ast-primitive? ast)
         (ast-primitive (ast-primitive-operator ast)
                        (ast-instantiate-list (ast-primitive-operands ast) x r)))
        ((ast-call? ast)
         (ast-call (ast-instantiate (ast-call-operator ast) x r)
                   (ast-instantiate-list (ast-call-operands ast) x r)))
        ((ast-lambda? ast)
         (let ((formals (ast-lambda-formals ast)))
           (if (set-has? formals x)
               ast ;; x occurs bound: don't touch its uses.
               (ast-lambda formals
                           (ast-instantiate (ast-lambda-body ast) x r)))))
        ((ast-let? ast)
         ;; Always instantiate in the bound form.  Instantiate in the body
         ;; only if the let variable is not the one we are replacing.  Don't
         ;; rename the bound variable.
         (let* ((bound-name (ast-let-bound-name ast))
                (old-bound-form (ast-let-bound-form ast))
                (new-bound-form (ast-instantiate old-bound-form x r))
                (old-body (ast-let-body ast))
                (new-body (if (eq? bound-name x)
                              old-body
                              (ast-instantiate old-body x r))))
           (ast-let bound-name
                    new-bound-form
                    new-body)))
        ((ast-sequence? ast)
         (ast-sequence (ast-instantiate (ast-sequence-first ast) x r)
                       (ast-instantiate (ast-sequence-second ast) x r)))))

;;; An extension of ast-instantiate to a list of ASTs: return the list
;;; of rewriten ASTs in order.
(define-constant (ast-instantiate-list asts x r)
  (map (lambda (ast) (ast-instantiate ast x r))
       asts))




;;;; AST leafness.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Return non-#f iff the given AST is a leaf expression.
(define-constant (ast-leaf? ast)
  (cond ((ast-literal? ast)
         #t)
        ((ast-variable? ast)
         #t)
        ((ast-define? ast)
         (ast-leaf? (ast-define-body ast)))
        ((ast-if? ast)
         (and (ast-leaf? (ast-if-condition ast))
              (ast-leaf? (ast-if-then ast))
              (ast-leaf? (ast-if-else ast))))
        ((ast-set!? ast)
         (ast-leaf? (ast-set!-body ast)))
        ((ast-while? ast)
         (and (ast-leaf? (ast-while-guard ast))
              (ast-leaf? (ast-while-body ast))))
        ((ast-primitive? ast)
         (ast-leaf?-list (ast-primitive-operands ast)))
        ((ast-call? ast)
         #f)
        ((ast-lambda? ast)
         (ast-leaf? (ast-lambda-body ast)))
        ((ast-let? ast)
         (and (ast-leaf? (ast-let-bound-form ast))
              (ast-leaf? (ast-let-body ast))))
        ((ast-sequence? ast)
         (and (ast-leaf? (ast-sequence-first ast))
              (ast-leaf? (ast-sequence-second ast))))))

;;; An extension of ast-leaf? to a list of ASTs: return non-#f iff the ASTs in
;;; the given list are all leaves.
(define-constant (ast-leaf?-list asts)
  (cond ((null? asts)
         #t)
        ((ast-leaf? (car asts))
         (ast-leaf?-list (cdr asts)))
        (else
         #f)))




;;;; AST call simplification.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; The transformations here rely on the caller being alpha-converted already:
;;; any inlining might result in nonglobal capture otherwise.

;;; A call where the operator is a closure literal (usually obtained by a
;;; previous optimization) can be easily rewritten into a let binding each
;;; formal as a local variable and having the lambda body as the let body,
;;; as long as the closure environment is empty.
;;; Some optimizations can probably be performed even with non-empty
;;; closure environments but this is not implemented yet, as the way
;;; environments are represented will change in the future.
;;;
;;; Most of the let forms introduced by this rewrite can be optimized away
;;; in a later pass.

;;; Return a rewritten call having the given closure as the original (literal)
;;; operator, and the ASTs in the given list as operands.
(define-constant (ast-simplify-known-closure-call-helper closure actuals)
  ;; FIXME: would it be a problem for termination if I used ast-optimize instead
  ;; of ast-simplify-known-closure-call-helper in recursive calls?  Would it
  ;; help?
  (let ((environment (interpreted-closure-environment closure))
        (formals (interpreted-closure-formals closure))
        (body (interpreted-closure-body closure)))
    (cond ((non-null? environment)
           ;; We currently don't rewrite if the environment is non-empty.
           (ast-call (ast-literal closure) actuals))
          ((<> (length formals) (length actuals))
           ;; This call will fail if reached: don't rewrite it.
           ;; FIXME: warn in a cleaner way.
           (display `(warning: invalid in-arity ,(length actuals)
                               for call to in-arity ,(length formals)
                               ,closure
                               with actuals ,actuals))
           (newline)
           (ast-call (ast-literal closure) actuals))
          ((closure-wrapper? closure)
           ;; Wrapper calls are easy to rewrite into a particularly efficient
           ;; AST.  This rewrite could be subsumed by others not yet
           ;; implemented, but this is very common and important to have from
           ;; the get go.
           (ast-rewrite-wrapper-call body actuals))
          (else
           ;; The environment is empty, and the argument number is correct:
           ;; rewrite into nested lets binding the closure formals to the call
           ;; actuals, and then evaluating the closure body.  alpha-convert the
           ;; closure content to avoid conflicts with local variables in the
           ;; caller.
           ;; (display `(inlining call to ,closure)) (newline)
           (let* ((new-formals (map (lambda (useless) (gensym)) formals))
                  (alist (zip formals new-formals))
                  (new-body (ast-alpha-convert-with body alist)))
             (ast-nested-let new-formals actuals new-body))))))

;;; I define a "wrapper" to be an empty-environment closure with n formals,
;;; whose entire body consists of either:
;;; (a) a primitive with the formals as its operands, all used, in the same
;;;     order;
;;; (b) a call with a leaf operator where no formal occurs free in the operator,
;;;     and the operands are like in the previous case.
;;; Notice that the second case has no restriction on operator effects: even in
;;; rewritten form the order of effects doesn't change.
;;;
;;; The leafness restriction is unfortunate, but is an easy way to avoid
;;; infinite expansion in case the operator contains recursive calls to itself
;;; or to the closure containing it.
;;;
;;; Wrappers are common and calls to them can be rewritten efficiently.  This
;;; is an easy check to make, which may be subsumed by other rewrites -- however
;;; those rewrites, still to implement, are much more complex.
;;;
;;; Notice that wrapper closures by definition have a leaf body and an empty
;;; environment, so they are always considered for inlining.
(define-constant (closure-wrapper? closure)
  (let ((environment (interpreted-closure-environment closure))
        (formals (interpreted-closure-formals closure))
        (body (interpreted-closure-body closure)))
    (cond ((non-null? environment)
           ;; This could be generalized, but is probably not worth the trouble
           ;; yet: right now we refuse to consider a closure to be a wrapper if
           ;; it has any nonlocals.
           #f)
          ((and (ast-primitive? body)
                (ast-wrapper-arguments? formals (ast-primitive-operands body)))
           ;; The primitive case as defined above.
           #t)
          ((and (ast-call? body)
                (ast-leaf? (ast-call-operator body))
                (for-all? (lambda (formal)
                            (not (ast-has-free? (ast-call-operator body)
                                                formal)))
                          formals)
                ;; No restrictions on operator effects here.  On purpose.
                (ast-wrapper-arguments? formals (ast-call-operands body)))
           ;; The call case as defined above.
           #t)
          (else
           ;; In any other case, the body is not a wrapper.
           #f))))

;;; A helper for closure-wrapper?.
;;; Return non-#f iff the given actuals respect the wrapper definition above,
;;; agreeing with the given formals.
(define-constant (ast-wrapper-arguments? formals actuals)
  (cond ((null? formals)
         ;; If both lists are empty the two in-arities agree.
         (null? actuals))
        ((null? actuals)
         ;; Different in-arities.
         #f)
        ((and (ast-variable? (car actuals))
              (eq? (car formals)
                   (ast-variable-name (car actuals))))
         (ast-wrapper-arguments? (cdr formals) (cdr actuals)))
        (else
         ;; Non-variable actual, or variable not matching the
         ;; formal in its position.
         #f)))

;;; Return non-#f iff the argument is an interpreted closure satisfying the (a)
;;; case of the definition above.  This is convenient to use below in the
;;; compiler, as "primitive wrappers" can be compiled to efficient code.
(define-constant (closure-primitive-wrapper? thing)
  (if (not (interpreted-closure? thing))
      #f
      (let ((environment (interpreted-closure-environment thing))
            (formals (interpreted-closure-formals thing))
            (body (interpreted-closure-body thing)))
        (and (ast-primitive? body)
             (ast-wrapper-arguments? formals (ast-primitive-operands body))))))

;;; Return a rewritten a wrapper call.  We assume that the body is a wrapper,
;;; and that the actuals respect the wrapper in-arity; notice that we even
;;; ignore the operator formal names.
(define-constant (ast-rewrite-wrapper-call body actuals)
  (cond ((ast-primitive? body)
         ;; We don't need any let, or even alpha-conversion: any call to a
         ;; primitive wrapper with the correct in-arity can be rewritten to
         ;; [primitive p . actuals], as long as the actuals are not reordered;
         ;; this is correct even with effects.
         (ast-primitive (ast-primitive-operator body) actuals))
        ((ast-call? body)
         ;; As long as this is a procedure wrapper we can do like in the
         ;; primitive case.  This does require the operator not to have effects,
         ;; but that has been checked already by this procedure's caller.
         (ast-call (ast-call-operator body) actuals))
        (else
         ;; This shouldn't happen.
         (error `(ast-rewrite-wrapper-call: operator ,body not a
                                            wrapper body)))))


;;; Given a list of bound variables, a list of actuals and a body, build nested
;;; let ASTs, evaluating the actuals left-to-right.  This is similar to a let*
;;; in Lisp, with a different syntax.  Assume that the formals and the actual
;;; ASTs have the same length.
;;; We can afford simple nested bindings here since procedure formals are
;;; guaranteed to be all different.
(define-constant (ast-nested-let formals actual-asts body-ast)
  (if (null? formals)
      body-ast
      (ast-let (car formals) (car actual-asts)
               (ast-nested-let (cdr formals) (cdr actual-asts)
                               body-ast))))

;;; Return the rewritten form of a call to the lambda operator shaped like
;;; ((lambda FORMALS BODY-AST) . ACTUALS-ASTS) , using nested let forms.
(define-constant (ast-simplify-lambda-call formals actual-asts body-ast)
  (if (<> (length formals) (length actual-asts))
      ;; Don't rewrite in case of arity mismatch: the code will fail
      ;; if reached.
      (begin
        ;; FIXME: warn more cleanly.
        (display `(WARNING: in-arity mismatch in call to lambda
                            with formals ,formals and body ,body-ast))
        (newline)
        (ast-call (ast-lambda formals body-ast) actual-asts))
      ;; Generate two layers of nested lets, the outer layer binding fresh
      ;; identifiers to actuals, the inner layer binding the original lambda
      ;; formals to the fresh identifiers of the outer layer.  This may be
      ;; needed to avoid capture, and is the same trick used to rewrite
      ;; one Lisp-style multiple-binding let into nested AST-style
      ;; one-binding lets.
      (let* ((fresh-variables (map (lambda (useless) (gensym)) formals))
             (fresh-variable-asts (map ast-variable fresh-variables)))
        (ast-nested-let (append fresh-variables formals)
                        (append actual-asts fresh-variable-asts)
                        body-ast))))

;;; Return a copy of the given AST where calls to closure literals are
;;; rewritten into let forms where possible.  The AST is assumed to be
;;; already alpha-converted.
(define-constant (ast-simplify-calls ast)
  (cond ((ast-literal? ast)
         ast)
        ((ast-variable? ast)
         ast)
        ((ast-define? ast)
         (ast-define (ast-define-name ast)
                     (ast-simplify-calls (ast-define-body ast))))
        ((ast-if? ast)
         (ast-if (ast-simplify-calls (ast-if-condition ast))
                 (ast-simplify-calls (ast-if-then ast))
                 (ast-simplify-calls (ast-if-else ast))))
        ((ast-set!? ast)
         (ast-set! (ast-set!-name ast)
                   (ast-simplify-calls (ast-set!-body ast))))
        ((ast-while? ast)
         (ast-while (ast-simplify-calls (ast-while-guard ast))
                    (ast-simplify-calls (ast-while-body ast))))
        ((ast-primitive? ast)
         (ast-primitive (ast-primitive-operator ast)
                        (ast-simplify-calls-list (ast-primitive-operands ast))))
        ((ast-call? ast)
         (let (;; Simplifying the operator seems perfunctory if we consider the
               ;; kind of ASTs this procedure is used on.  Anyway it might be
               ;; profitable in the future, with ASTs of a different shape.
               (simplified-operator
                (ast-simplify-calls (ast-call-operator ast)))
               ;; Simplifying the operands is always reasonable, instead: they
               ;; may contain calls.
               (simplified-operands
                (ast-simplify-calls-list (ast-call-operands ast))))
           (cond ((ast-lambda? simplified-operator)
                  ;;; This can always be rewritten into nested let forms, as
                  ;;; long as the arity matches.
                  (ast-simplify-lambda-call (ast-lambda-formals
                                             simplified-operator)
                                            simplified-operands
                                            (ast-lambda-body
                                             simplified-operator)))
                 ;; FIXME: support compiled closures as well, somehow.
                 ((and (ast-literal? simplified-operator)
                       (interpreted-closure? (ast-literal-value simplified-operator))
                       (or ;; This may be too aggressive: I currently inline
                           ;; every call to a known leaf closure, independently
                           ;; from the body size.
                           (ast-leaf? (interpreted-closure-body (ast-literal-value
                                                                 simplified-operator)))
                           ;; In order to make this not too aggressive as well,
                           ;; I require procedure wrapper bodies to be leaves;
                           ;; otherwise a procedure wrapper which is recursive
                           ;; on the operator side would cause an infinite
                           ;; expansion here.
                           (closure-wrapper? (ast-literal-value
                                              simplified-operator))))
                  (ast-simplify-known-closure-call-helper (ast-literal-value
                                                           simplified-operator)
                                                          simplified-operands))
                 (else
                  (ast-call simplified-operator simplified-operands)))))
        ((ast-lambda? ast)
         (ast-lambda (ast-lambda-formals ast)
                     (ast-simplify-calls (ast-lambda-body ast))))
        ((ast-let? ast)
         (ast-let (ast-let-bound-name ast)
                  (ast-simplify-calls (ast-let-bound-form ast))
                  (ast-simplify-calls (ast-let-body ast))))
        ((ast-sequence? ast)
         (ast-sequence (ast-simplify-calls (ast-sequence-first ast))
                       (ast-simplify-calls (ast-sequence-second ast))))))

;;; An extension of ast-simplify-calls to a list of ASTs: return the list
;;; of rewriten ASTs in order.
(define-constant (ast-simplify-calls-list asts)
  (map ast-simplify-calls asts))




;;;; AST optimization.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Return an equivalent rewritten version of the given AST, which is assumed to
;;; be alpha-converted, knowing that the given set-of-list of variables is
;;; bound.
;;; The most important optimizations are for the let case, which uses the helper
;;; below.
(define-constant (ast-optimize-helper ast bounds)
  (cond ((ast-literal? ast)
         ast)
        ((ast-variable? ast)
         ast)
        ((ast-define? ast)
         (ast-define (ast-define-name ast)
                     (ast-optimize-helper (ast-define-body ast) bounds)))
        ((ast-if? ast)
         (ast-optimize-if (ast-if-condition ast)
                          (ast-if-then ast)
                          (ast-if-else ast)
                          bounds))
        ((ast-set!? ast)
         (ast-optimize-set! (ast-set!-name ast)
                            (ast-optimize-helper (ast-set!-body ast) bounds)
                            bounds))
        ((ast-while? ast)
         (ast-optimize-while (ast-optimize-helper (ast-while-guard ast)
                                                  bounds)
                             (ast-while-body ast)
                             bounds))
        ((ast-primitive? ast)
         (ast-optimize-primitive (ast-primitive-operator ast)
                                 (ast-optimize-helper-list
                                     (ast-primitive-operands ast)
                                     bounds)))
        ((ast-call? ast)
         (ast-call (ast-optimize-helper (ast-call-operator ast) bounds)
                   (ast-optimize-helper-list (ast-call-operands ast) bounds)))
        ((ast-lambda? ast)
         (let ((formals (ast-lambda-formals ast)))
           (ast-lambda formals
                       (ast-optimize-helper (ast-lambda-body ast)
                                            (set-unite bounds formals)))))
        ((ast-let? ast)
         (let ((bound-name (ast-let-bound-name ast)))
           (ast-optimize-let bound-name
                             (ast-optimize-helper (ast-let-bound-form ast) bounds)
                             (ast-optimize-helper (ast-let-body ast)
                                                  (set-with bounds bound-name))
                             bounds)))
        ((ast-sequence? ast)
         (let ((optimized-first (ast-optimize-helper (ast-sequence-first ast)
                                                     bounds))
               (optimized-second (ast-optimize-helper (ast-sequence-second ast)
                                                      bounds)))
           (ast-optimize-sequence optimized-first
                                  optimized-second
                                  bounds)))))

;;; An extension of ast-optimize-helper to a list of ASTs: return the list
;;; of rewriten ASTs in order.
(define-constant (ast-optimize-helper-list asts bounds)
  (map (lambda (ast) (ast-optimize-helper ast bounds))
       asts))

;;; A helper for ast-optimize-helper in the sequence case.  Assume that both the
;;; subforms are already optimized.
(define-constant (ast-optimize-sequence optimized-first optimized-second bounds)
  (cond ((and (ast-variable? optimized-first)
              (ast-equal? optimized-first optimized-second))
         ;; Rewrite [sequence [variable x] [variable x]] into [variable x].
         ;; This is correct even if x is not known to be bound, as no effects
         ;; are removed: the removed (second) reference is guaranteed not to
         ;; have observable effects after the first reference succeeds.
         ;; Other optimizations give opportunity for this case to apply.
         optimized-first)
        ((not (ast-effectful? optimized-first bounds))
         ;; The first form in the sequence has no effect: rewrite to the second
         ;; form only.
         optimized-second)
        (else
         ;; Fallback case: keep the sequence.
         (ast-sequence optimized-first
                       optimized-second))))

;;; A helper for ast-optimize-helper in the set! case.  The body should already
;;; be optimized.
(define-constant (ast-optimize-set! name body bounds)
  ;; There isn't much I can do here without going to extreme lengths.
  (cond ((and (ast-variable? body)
              (eq? (ast-variable-name body) name)
              (set-has? bounds name))
         ;; An easy case to optimize is [set! x [variable x]], which we can
         ;; rewrite into [literal #<nothing>] as long as x is non-globally bound
         ;; (otherwise the reference to x would be effectful, which would make
         ;; its removal incorrect).  This occurs, for example, in the
         ;; macroexpansion of (letrec ((a a)) a), which is a way of obtaining
         ;; #<undefined> as a result.  Notice that [set! x x] is effectful when
         ;; x is a global constant, and therefore we do not remove it in that
         ;; case; the condition checks whether the variable is bound in the
         ;; non-global environement only, on purpose.
         (ast-literal (begin)))
        (else
         ;; Fallback case, in which we optimize nothing.
         (ast-set! name body))))

;;; A helper for ast-optimize-helper in the let case, which is the most complex.
;;; This assumes that both subforms are alpha-converted.
(define-constant (ast-optimize-let bound-name bound-form body bounds)
  (cond ((ast-sequence? bound-form)
         ;; Rewrite [let x [sequence E1 E2] E3] into [sequence E1 [let x E2 E3]]
         ;; , which may enable further optimizations.  Notice that moving E1 out
         ;; of the let form doesn't change the set of bound variables at any
         ;; program point, as the bound x is not visible in E1.
         ;; So, do the change...
         (let ((rewritten
                (ast-sequence (ast-sequence-first bound-form)
                              (ast-let bound-name
                                       (ast-sequence-second bound-form)
                                       body))))
           ;; ...and then re-optimize the rewritten sequence.  This may trigger
           ;; the same rewrite on a bound-form sub-sequence, or other
           ;; optimizations; in particular the bound form, now smaller, may have
           ;; been reduced to a variable or a literal.
           (ast-optimize-helper rewritten bounds)))
        ((not (ast-has-free? body bound-name))
         ;; The bound variable is not used in the body.  Rewrite the let into
         ;; a sequence and optimize it further.
         (ast-optimize-helper (ast-sequence bound-form body)
                              bounds))
        ((and (ast-literal? bound-form)
              (not (ast-has-assigned? body bound-name)))
         ;; The variable is bound to a literal, without being assigned:
         ;; fold the literal into the body and optimize it further.
         (let ((folded-body (ast-instantiate body bound-name bound-form)))
           (ast-optimize-helper folded-body bounds)))
        ((and (ast-variable? bound-form)
              (not (ast-has-assigned? body bound-name))
              (not (ast-has-assigned? body (ast-variable-name bound-form)))
              (or (set-has? bounds (ast-variable-name bound-form))
                  (constant? (ast-variable-name bound-form))))
         ;; The variable is bound to another variable, with neither being
         ;; assigned in the body.  We can reduce the entire let AST to a body
         ;; with the bound variable replaced by the other.  Here we rely on the
         ;; body being alpha-converted to be sure not to capture the substituted
         ;; variable.
         ;; The or clause is important: this rewriting is only valid if we are
         ;; sure that referencing the bound-form variable will not have effects;
         ;; it being undefined would trigger an error, and we don't want to move
         ;; the error point.
         (let ((new-body (ast-instantiate body bound-name bound-form)))
           (ast-optimize-helper new-body bounds)))
        ((and (ast-variable? body)
              (eq? bound-name (ast-variable-name body)))
         ;; Rewrite [let x E [variable x]] into E , without any restriction on
         ;; the shape of E , on x or on effects.
         ;; This rewrite could be subsumed by more general rules which are not
         ;; implemented yet but at least this case is easy to optimize, and
         ;; an opportunity to improve tailness.  It can occur as a consequence
         ;; of other rewrites.
         bound-form)
        (else
         ;; Default case: keep the let AST in our rewriting.
         (ast-let bound-name
                  bound-form
                  (ast-optimize-helper body (set-with bounds bound-name))))))

;;; A helper for ast-optimize-helper in the if case.
(define-constant (ast-optimize-if condition then else bounds)
  (ast-optimize-if-helper (ast-optimize-helper condition bounds)
                          (ast-optimize-helper then bounds)
                          (ast-optimize-helper else bounds)
                          bounds))

;;; A helper for ast-optimize-if, requiring every AST sub to be already optimized.
(define-constant (ast-optimize-if-helper condition then else bounds)
  (cond ((ast-sequence? condition)
         ;; Rewrite [if [sequence E1 E2] E3 E4] into
         ;; [sequence E1 [if E2 E3 E4]], and optimize the result.  This may
         ;; lead to further optimizations, particularly if the condition
         ;; eventually reduces to a constant.  The set of bound variables
         ;; doesn't change at any point.
         (let ((sequence
                (ast-sequence (ast-sequence-first condition)
                              (ast-if (ast-sequence-second condition)
                                      then
                                      else))))
           (ast-optimize-helper sequence bounds)))
        ((and (ast-primitive? condition)
              (eq? (ast-primitive-operator condition)
                   primitive-boolean-canonicalize))
         ;; Rewrite [if [primitive boolean-canonicalize E1] E2 E3] into
         ;; [if E1 E2 E3], and optimize the result.  Boolean canonicalization
         ;; is a waste of time in this position.
         (ast-optimize-if-helper (car (ast-primitive-operands condition))
                                 then
                                 else
                                 bounds))
        ((and (ast-primitive? condition)
              (eq? (ast-primitive-operator condition)
                   primitive-not))
         ;; Rewrite [if [primitive not E1] E2 E3] into [if E1 E3 E2], and
         ;; optimize the result.
         (ast-optimize-if-helper (car (ast-primitive-operands condition))
                                 else
                                 then
                                 bounds))
        ((and (ast-literal? condition)
              (ast-literal-value condition))
         ;; The condition has been simplified to non-#f: rewrite
         ;; [if [literal non-#f] E1 E2] into E1.
         then)
        ((ast-literal? condition)
         ;; The condition has been simplified to #f, since we didn't
         ;; get to the previous clause: rewrite [if [literal #f] E1 E2] into E2.
         else)
        ((ast-equal? then else)
         ;; The two branches are equal, so we don't need to have a conditional
         ;; at all: turn it into a sequence of the condition and one branch;
         ;; this will usually be further optimizable as the condition tends not
         ;; to have effects.
         ;; Rewrite [if E1 E2 E2] into [sequence E1 E2].
         (ast-optimize-helper (ast-sequence condition
                                            then)
                              bounds))
        ((and (ast-equal? condition then)
              (ast-literal? else)
              (not (ast-literal-value else))
              (not (ast-effectful? condition bounds)))
         ;; The condition has no effects and is equal to the then branch, with
         ;; an else branch which is the literal #f.  This occurs in the
         ;; expansion of (and X X) with a non-effectul X.
         ;; Rewrite [if E1 E1 [literal #f]] into E1.
         condition)
        ((and (ast-equal? condition else)
              (ast-literal? then)
              (eq? (ast-literal-value then) #t) ;; Exactly the canonical #t.
              (not (ast-effectful? condition bounds)))
         ;; The condition has no effects and is equal to the else branch, with
         ;; an then branch which is the literal #t -- exactly that canonical
         ;; boolean, not any other non-#f value.  This occurs in the
         ;; expansion of the non-Lispy (or X X) with a non-effectul X.
         ;; Rewrite [if E1 [literal #t] E1] into
         ;; [primitive boolean-canonicalize E1].
         ;; This is provided for symmetry with the previous case, mostly for
         ;; fun.
         (ast-optimize-helper (ast-primitive primitive-boolean-canonicalize
                                             (list condition))
                              bounds))
        ((and (ast-primitive? else)
              (eq? (ast-primitive-operator else)
                   primitive-boolean-canonicalize)
              (ast-equal? condition
                          (car (ast-primitive-operands else)))
              (ast-literal? then)
              (eq? (ast-literal-value then) #t) ;; Exactly the canonical #t.
              (not (ast-effectful? condition bounds)))
         ;; A generalization of the previous case to the expansion of non-Lispy
         ;; (or X X ... X).
         ;; Rewrite [if E1 [literal #t] [primitive boolean-canonicalize E1]]
         ;; into [primitive boolean-canonicalize E1].
         (ast-optimize-helper (ast-primitive primitive-boolean-canonicalize
                                             (list condition))
                              bounds))
        ((and (ast-literal? then)
              (ast-literal? else)
              (eq? (ast-literal-value then) #t) ;; The canonical #t.
              (not (ast-literal-value else)))
         ;; Rewrite [if E #t #f] into [primitive boolean-canonicalize E]; notice
         ;; that there is no requirement on effectfulness or on the shape of the
         ;; condition.
         ;; This doesn't only occur in dumb code written by human beginners: it
         ;; occurs, for example, in the expansion of non-Lispy (or X #f), which
         ;; may well come from the expansion of another macro.
         (ast-optimize-helper (ast-primitive primitive-boolean-canonicalize
                                             (list condition))
                              bounds))
        ((and (ast-literal? then)
              (ast-literal? else)
              (not (ast-literal-value then))
              (eq? (ast-literal-value else) #t)) ;; The canonical #t.
         ;; Rewrite [if E #f #t] into [primitive not E]; again there is no
         ;; requirement on effectfulness or on the shape of the condition.
         ;; This is symmetrical with respect to the previous case.
         (ast-optimize-helper (ast-primitive primitive-not
                                             (list condition))
                              bounds))
        (else
         ;; Generic case.  Keep both branches, each optimized separately.
         (ast-if condition
                 then
                 else))))

;;; A helper for ast-optimize-helper in the while case.  Only the guard
;;; needs to be already optimized.
(define-constant (ast-optimize-while optimized-guard body bounds)
  (cond ((ast-sequence? optimized-guard)
         ;; Rewrite [while [sequence E1 E2] E3] into
         ;; [sequence E1 [while E2 [sequence E3 E1]]] and optimize further.
         ;; The bound variable set doesn't change at any program point.
         (let* ((first (ast-sequence-first optimized-guard))
                (second (ast-sequence-second optimized-guard))
                (sequence (ast-sequence first
                                        (ast-while second
                                                   (ast-sequence body
                                                                 first)))))
           (ast-optimize-helper sequence bounds)))
        ((and (ast-primitive? optimized-guard)
              (eq? (ast-primitive-operator optimized-guard)
                   primitive-boolean-canonicalize))
         ;; Rewrite [while [primitive boolean-canonicalize E1] E2] into
         ;; [while E1 E2], and optimize the result.  Boolean canonicalization
         ;; is a waste of time in this position.
         (ast-optimize-while (car (ast-primitive-operands optimized-guard))
                             body
                             bounds))
        ((and (ast-literal? optimized-guard)
              (not (ast-literal-value optimized-guard)))
         ;; Replace [while [literal #f] E] with [literal #<nothing>].  This is
         ;; correct with any E, even if it has effects.  Notice that we can't
         ;; simplify while with a constantly non-#f guard.
         (ast-literal (begin)))
        (else
         ;; Keep the while form.
         (ast-while optimized-guard
                    (ast-optimize-helper body bounds)))))

;;; Return a rewritten version of a primitive use with the given primitive
;;; operator and the given list of AST operands, already rewritten.
(define-constant (ast-optimize-primitive primitive operands)
  ;; Here I can assume the primitive in-arity to be respected: there would
  ;; have been an error at AST creation time otherwise.
  (cond ;; Successor and predecessor; multiplication, division and remainder by
        ;; two.
        ((and (eq? primitive primitive-primordial-+) (ast-one? (car operands)))
         ;; [primitive primordial-+ 1 E] ==> [primitive 1+ E]
         (ast-optimize-primitive primitive-1+ (list (cadr operands))))
        ((and (eq? primitive primitive-primordial-+) (ast-one? (cadr operands)))
         ;; [primitive primordial-+ E 1] ==> [primitive 1+ E]
         (ast-optimize-primitive primitive-1+ (list (car operands))))
        ((and (eq? primitive primitive-primordial-*) (ast-two? (car operands)))
         ;; [primitive primordial-* 2 E] ==> [primitive 2* E]
         (ast-optimize-primitive primitive-2* (list (cadr operands))))
        ((and (eq? primitive primitive-primordial-*) (ast-two? (cadr operands)))
         ;; [primitive primordial-* E 2] ==> [primitive 2* E]
         (ast-optimize-primitive primitive-2* (list (car operands))))
        ((and (eq? primitive primitive-primordial--) (ast-one? (cadr operands)))
         ;; [primitive primordial-- E 1] ==> [primitive 1- E]
         (ast-optimize-primitive primitive-1- (list (car operands))))
        ((and (eq? primitive primitive-primordial-+)
              (ast-minus-one? (car operands)))
         ;; [primitive primordial-+ -1 E] ==> [primitive 1- E]
         (ast-optimize-primitive primitive-1- (list (cadr operands))))
        ((and (eq? primitive primitive-primordial-+)
              (ast-minus-one? (cadr operands)))
         ;; [primitive primordial-+ E -1] ==> [primitive 1- E]
         (ast-optimize-primitive primitive-1- (list (car operands))))
        ((and (eq? primitive primitive-primordial--)
              (ast-minus-one? (cadr operands)))
         ;; [primitive primordial-- E -1] ==> [primitive 1+ E]
         (ast-optimize-primitive primitive-1+ (list (car operands))))
        ((and (eq? primitive primitive-primordial-/) (ast-two? (cadr operands)))
         ;; [primitive primordial-/ E 2] ==> [primitive 2/ E]
         (ast-optimize-primitive primitive-2/ (list (car operands))))
        ((and (eq? primitive primitive-quotient)
              (ast-two? (cadr operands)))
         ;; [primitive quotient E 2] ==> [primitive 2quotient E]
         (ast-optimize-primitive primitive-2quotient (list (car operands))))
        ((and (eq? primitive primitive-remainder) (ast-two? (cadr operands)))
         ;; [primitive remainder E 2] ==> [primitive 2remainder E]
         (ast-optimize-primitive primitive-2remainder (list (car operands))))
        ;; Zero tests.
        ((and (eq? primitive primitive-=) (ast-zero? (car operands)))
         ;; [primitive = 0 E] ==> [primitive zero? E]
         (ast-optimize-primitive primitive-zero? (list (cadr operands))))
        ((and (eq? primitive primitive-=) (ast-zero? (cadr operands)))
         ;; [primitive = E 0] ==> [primitive zero? E]
         (ast-optimize-primitive primitive-zero? (list (car operands))))
        ;; Sign tests.
        ((and (eq? primitive primitive-<) (ast-zero? (cadr operands)))
         ;; [primitive < E 0] ==> [primitive negative? E]
         (ast-optimize-primitive primitive-negative? (list (car operands))))
        ((and (eq? primitive primitive-<=) (ast-zero? (cadr operands)))
         ;; [primitive <= E 0] ==> [primitive non-positive? E]
         (ast-optimize-primitive primitive-non-positive? (list (car operands))))
        ((and (eq? primitive primitive->) (ast-zero? (cadr operands)))
         ;; [primitive > E 0] ==> [primitive positive? E]
         (ast-optimize-primitive primitive-positive? (list (car operands))))
        ((and (eq? primitive primitive->=) (ast-zero? (cadr operands)))
         ;; [primitive >= E 0] ==> [primitive non-negative? E]
         (ast-optimize-primitive primitive-non-negative? (list (car operands))))
        ((and (eq? primitive primitive-<) (ast-zero? (car operands)))
         ;; [primitive < 0 E] ==> [primitive positive? E]
         (ast-optimize-primitive primitive-positive? (list (cadr operands))))
        ((and (eq? primitive primitive-<=) (ast-zero? (car operands)))
         ;; [primitive <= 0 E] ==> [primitive non-negative? E]
         (ast-optimize-primitive primitive-non-negative? (list (cadr operands))))
        ((and (eq? primitive primitive->) (ast-zero? (car operands)))
         ;; [primitive > 0 E] ==> [primitive negative? E]
         (ast-optimize-primitive primitive-negative? (list (cadr operands))))
        ((and (eq? primitive primitive->=) (ast-zero? (car operands)))
         ;; [primitive >= 0 E] ==> [primitive non-positive? E]
         (ast-optimize-primitive primitive-non-positive? (list (cadr operands))))
        ;; Arithmetic with neutral operands.
        ;; Notice that we cannot, in general, rewrite primitives with absorbing
        ;; operands without removing effects; however neutral operands are fine.
        ((and (eq? primitive primitive-primordial-+) (ast-zero? (car operands)))
         ;; [primitive + 0 E] ==> E
         (cadr operands))
        ((and (eq? primitive primitive-primordial-+) (ast-zero? (cadr operands)))
         ;; [primitive + E 0] ==> E
         (car operands))
        ((and (eq? primitive primitive-primordial--) (ast-zero? (cadr operands)))
         ;; [primitive - E 0] ==> E
         (car operands))
        ((and (eq? primitive primitive-primordial-*) (ast-one? (car operands)))
         ;; [primitive * 1 E] ==> E
         (cadr operands))
        ((and (eq? primitive primitive-primordial-*) (ast-one? (cadr operands)))
         ;; [primitive * E 1] ==> E
         (car operands))
        ((and (eq? primitive primitive-primordial-/) (ast-one? (cadr operands)))
         ;; [primitive / E 1] ==> E
         (car operands))
        ;; Division and remainder by non-zero literals.
        ((and (eq? primitive primitive-primordial-/)
              (ast-non-zero? (cadr operands)))
         ;; [primitive / E NZ] ==> [primitive /-unsafe E NZ]
         (ast-optimize-primitive primitive-primordial-/-unsafe operands))
        ((and (eq? primitive primitive-quotient)
              (ast-non-zero? (cadr operands)))
         ;; [primitive quotient E NZ] ==> [primitive quotient-unsafe E NZ]
         (ast-optimize-primitive primitive-quotient-unsafe operands))
        ((and (eq? primitive primitive-remainder)
              (ast-non-zero? (cadr operands)))
         ;; [primitive remainder E NZ] ==> [primitive remainder-unsafe E NZ]
         (ast-optimize-primitive primitive-remainder-unsafe operands))
        ;; Other arithmetic simplification with particular literal operands.
        ((and (eq? primitive primitive-primordial--) (ast-zero? (car operands)))
         ;; [primitive - 0 E] ==> [primitive negate E]
         (ast-optimize-primitive primitive-negate (list (cadr operands))))
        ;; Nested arithmetic negation.
        ((and (eq? primitive primitive-negate)
              (ast-primitive? (car operands))
              (eq? (ast-primitive-operator (car operands)) primitive-negate))
         ;; [primitive negate [primitive negate E]] ==> E
         (car (ast-primitive-operands (car operands))))
        ;; Logical negation of another primitive.
        ((and (eq? primitive primitive-not)
              (ast-primitive? (car operands)))
         ;; [primitive not [primitive P . Es]].
         ;; Some primitives can be rewritten into a faster form when logically
         ;; negated.  Use the helper procedure for this.
         (let ((inner-primitive (ast-primitive-operator (car operands)))
               (inner-operands (ast-primitive-operands (car operands))))
           (ast-optimize-not-primitive inner-primitive inner-operands)))
        ((for-all? ast-literal? operands)
         ;; The actuals are all literals.  Try to evaluate the primitive use
         ;; at rewrite time, replacing it with a literal result.
         (ast-optimize-primitive-known-actuals primitive
                                               (map ast-literal-value
                                                    operands)))
        (else
         ;; Fallback case: we have nothing to rewrite.
         (ast-primitive primitive operands))))

;;; Return non-#f iff the given AST is a literal with the given value.
(define-constant (ast-literal-value? ast value)
  (and (ast-literal? ast)
       (eq? (ast-literal-value ast) value)))

;;; Return non-#f iff the given AST is repsectively the literal 0, 1, -1, 2.
(define-constant (ast-zero? ast)
  (ast-literal-value? ast 0))
(define-constant (ast-one? ast)
  (ast-literal-value? ast 1))
(define-constant (ast-minus-one? ast)
  (ast-literal-value? ast -1))
(define-constant (ast-two? ast)
  (ast-literal-value? ast 2))

;;; Return non-#f iff the given AST is a fixnum non-zero literal.
(define-constant (ast-non-zero? ast)
  (and (ast-literal? ast)
       (non-zero? (ast-literal-value ast))))

;;; A helper for ast-optimize-primitive.  Return the rewritten version of
;;; [primitive not [primitive PRIMITIVE . OPERANDS]].  The operands are already
;;; rewritten.
(define-constant (ast-optimize-not-primitive primitive operands)
  ;; Like in ast-optimize-primitive , I can assume that the in-arity is
  ;; respected.
  (cond ((eq? primitive primitive-not)
         ;; [primitive not [primitive not E]] ==> [boolean-canonicalize E].
         ;; A use of boolean-canonicalize as an condition will be rewritten
         ;; away by the if and while helpers.
         (ast-optimize-primitive primitive-boolean-canonicalize operands))
        ((eq? primitive primitive-boolean-canonicalize)
         ;; [primitive not [primitive boolean-canonicalize E]] ==>
         ;; [primitive not E].
         (ast-optimize-primitive primitive-not operands))
        ;; The following cases are obvious.
        ((eq? primitive primitive-null?)
         ;; [primitive not [primitive null? . Es]] ==> [primitive non-null? . Es]
         (ast-optimize-primitive primitive-non-null? operands))
        ((eq? primitive primitive-non-null?)
         ;; [primitive not [primitive non-null? . Es]] ==> [primitive null? . Es]
         (ast-optimize-primitive primitive-null? operands))
        ((eq? primitive primitive-eq?)
         ;; [primitive not [primitive eq? . Es]] ==> [primitive not-eq? . Es]
         (ast-optimize-primitive primitive-not-eq? operands))
        ((eq? primitive primitive-not-eq?)
         ;; [primitive not [primitive not-eq? . Es]] ==> [primitive eq? . Es]
         (ast-optimize-primitive primitive-eq? operands))
        ((eq? primitive primitive-zero?)
         ;; [primitive not [primitive zero? . Es]] ==> [primitive non-zero? . Es]
         (ast-optimize-primitive primitive-non-zero? operands))
        ((eq? primitive primitive-non-zero?)
         ;; [primitive not [primitive non-zero? . Es]] ==> [primitive zero? . Es]
         (ast-optimize-primitive primitive-zero? operands))
        ((eq? primitive primitive-positive?)
         ;; [primitive not [primitive positive? . Es]] ==> [primitive non-positive? . Es]
         (ast-optimize-primitive primitive-non-positive? operands))
        ((eq? primitive primitive-non-positive?)
         ;; [primitive not [primitive non-positive? . Es]] ==> [primitive positive? . Es]
         (ast-optimize-primitive primitive-positive? operands))
        ((eq? primitive primitive-negative?)
         ;; [primitive not [primitive negative? . Es]] ==> [primitive non-negative? . Es]
         (ast-optimize-primitive primitive-non-negative? operands))
        ((eq? primitive primitive-non-negative?)
         ;; [primitive not [primitive non-negative? . Es]] ==> [primitive negative? . Es]
         (ast-optimize-primitive primitive-negative? operands))
        ((eq? primitive primitive-=)
         ;; [primitive not [primitive = . Es]] ==> [primitive <> . Es]
         (ast-optimize-primitive primitive-<> operands))
        ((eq? primitive primitive-<>)
         ;; [primitive not [primitive <> . Es]] ==> [primitive = . Es]
         (ast-optimize-primitive primitive-= operands))
        ((eq? primitive primitive-<)
         ;; [primitive not [primitive < . Es]] ==> [primitive >= . Es]
         (ast-optimize-primitive primitive->= operands))
        ((eq? primitive primitive-<=)
         ;; [primitive not [primitive <= . Es]] ==> [primitive > . Es]
         (ast-optimize-primitive primitive-> operands))
        ((eq? primitive primitive->)
         ;; [primitive not [primitive > . Es]] ==> [primitive <= . Es]
         (ast-optimize-primitive primitive-<= operands))
        ((eq? primitive primitive->=)
         ;; [primitive not [primitive >= . Es]] ==> [primitive < . Es]
         (ast-optimize-primitive primitive-< operands))
        (else
         ;; Fallback case: don't rewrite anything.
         (ast-primitive primitive-not
                        (list (ast-primitive primitive operands))))))

;;; Another helper for ast-optimize-primitive.  Return an AST containing a
;;; rewritten primitive use of the given primitive with the given values (all
;;; known at rewrite time) as actuals; in some cases we can evaluate the
;;; primitive use at rewrite time, and replace it with the result as a literal.
(define-constant (ast-optimize-primitive-known-actuals primitive values)
  ;; Again I can assume that the in-arity is respected, but not necessarily the
  ;; actual types.
  (if (ast-statically-rewritable-primitive-use? primitive values)
      (ast-literal (apply-primitive primitive values))
      (ast-primitive primitive (map ast-literal values))))

;;; Given a primitive and a list of actual values return non-#f iff the use is
;;; known to be statically rewritable.
(define-constant (ast-statically-rewritable-primitive-use? primitive values)
  (let outer-loop ((signatures ast-statically-rewritable-primitive-signatures))
    (if (null? signatures)
        #f
        (let* ((signature (car signatures))
               (a-primitive (car signature))
               (conditions (cdr signature)))
          ;; In the inner loop it's convenient to iterate on a list of
          ;; predicates; so instead of a primitive object I will use as the
          ;; first element a predicate checking whether its argument is the
          ;; required primitive.  The list of values to match with the list of
          ;; predicates contains the primitives as the first element, followed
          ;; by the actual values.
          (let inner-loop ((conditions (cons (lambda (p) (eq? a-primitive p))
                                             conditions))
                           (values (cons primitive values)))
            (cond ((and (null? conditions) (null? values))
                   ;; Every condition was satisfied and there are no excess
                   ;; actuals: the signature matches.
                   #t)
                  ((null? conditions)
                   ;; No more conditions, but still remaining actuals.
                   (display `(WARNING: invalid signature: too many actuals for ,primitive)) (newline)
                   (outer-loop (cdr signatures)))
                  ((null? values)
                   ;; No more actuals, but still remaining conditions.
                   (display `(WARNING: invalid signature: not enough actuals for ,primitive)) (newline)
                   (outer-loop (cdr signatures)))
                  (((car conditions) (car values))
                   ;; The first condition on the first actual is satisfied.
                   ;; Check the others.
                   (inner-loop (cdr conditions) (cdr values)))
                  (else
                   ;; The first condition on the first actual is not satisfied.
                   ;; Leave this signature and try with the next.
                   (outer-loop (cdr signatures)))))))))


;;; Return non-#f iff the argument is a fixnum and different from 0.  Never
;;; fail.
(define-constant (non-zero-fixnum? x)
  (and (fixnum? x)
       (non-zero? x)))

;;; An unordered list of lists.  Each inner list contains a primitive, and then
;;; one procedure per primitive argument; the procedure is a predicate never
;;; failing and returning #t if the argument is suitable for the primitive, and
;;; safe to evaluate at rewrite time.
;;; Primitives not occurring here ar not candidates for rewrite-time evaluation.
;;;
;;; The outer list is walked sequentially, looking for the first match; it is
;;; possible for a primitive to appear in multiple lists, and that could be
;;; useful for primitives with multiple "safe signatures".
(define-constant ast-statically-rewritable-primitive-signatures
  (list ;; Type checking.
        (list primitive-null? anything?)
        (list primitive-non-null? anything?)
        (list primitive-fixnum? anything?)
        (list primitive-character? anything?)
        (list primitive-symbol? anything?)
        (list primitive-non-symbol? anything?)
        (list primitive-cons? anything?)
        (list primitive-non-cons? anything?)
        (list primitive-box? anything?)
        (list primitive-primitive? anything?)
        (list primitive-closure? anything?)
        (list primitive-vector? anything?)
        (list primitive-ast? anything?)
        (list primitive-macro? anything?)
        (list primitive-boolean? anything?)
        (list primitive-eof? anything?)
        (list primitive-nothing? anything?)
        (list primitive-undefined? anything?)

        ;; Case checking.  It's probably not worth the trouble to do this for
        ;; ASTs.
        (list primitive-zero? fixnum?)
        (list primitive-non-zero? fixnum?)
        (list primitive-positive? fixnum?)
        (list primitive-non-positive? fixnum?)
        (list primitive-negative? fixnum?)
        (list primitive-non-negative? fixnum?)

        ;; Generic comparisons.
        (list primitive-eq? anything? anything?)
        (list primitive-not-eq? anything? anything?)

        ;; Fixnum arithmetic.
        (list primitive-1+ fixnum?)
        (list primitive-1- fixnum?)
        (list primitive-negate fixnum?)
        (list primitive-primordial-+ fixnum? fixnum?)
        (list primitive-primordial-- fixnum? fixnum?)
        (list primitive-primordial-* fixnum? fixnum?)
        (list primitive-primordial-/ fixnum? non-zero-fixnum?)
        (list primitive-remainder fixnum? non-zero-fixnum?)

        ;; Fixnum comparisons.
        (list primitive-= fixnum? fixnum?)
        (list primitive-<> fixnum? fixnum?)
        (list primitive-< fixnum? fixnum?)
        (list primitive-<= fixnum? fixnum?)
        (list primitive-> fixnum? fixnum?)
        (list primitive->= fixnum? fixnum?)

        ;; Boolean operations.
        (list primitive-not anything?)
        (list primitive-boolean-canonicalize anything?)

        ;; Conses.
        ;; It is *not* safe to evaluate cons at rewrite time, as it needs to
        ;; allocate a different fresh object at every use.  More subtly, it's
        ;; also unsafe to evaluate selectors at rewrite time, as the data
        ;; structures involved might be destructively updated at run time
        ;; between initialization and selection.

        ;; Boxes.
        ;; The comment above about cons selectors applies to boxes as well.
        ))




;;;; AST optimization driver.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Return an optimized version of the given AST where the given set-as-list of
;;; variables is bound.
(define-constant (ast-optimize-procedure ast-0 bounds)
  (let* (;; Fold global constants into the AST.  This will introduce, in
         ;; particular, closure literals as operators.
         ;;(_ (display `(ast-0: ,ast-0)) (newline))
         (ast-1 (ast-global-fold ast-0 bounds))
         ;;(_ (display `(ast-1: ,ast-1)) (newline))
         ;; Alpha-convert every variable bound by the AST: we are about to
         ;; introduce new let bindings, and we need to prevent capture.
         (ast-2 (ast-alpha-convert ast-1))
         ;;(_ (display `(ast-2: ,ast-2)) (newline))
         ;; Translate calls to closures literals into let forms,
         ;; alpha-converting the inlined procedures.  This should make almost
         ;; all primitives explicit in the AST eliminating closure wrappers for
         ;; primitives, at the cost of introducing many redundant lets.
         ;; ast-3 will still be alpha-converted, with all bound variables
         ;; different from one another.
         (ast-3 (ast-simplify-calls ast-2))
         ;;(_ (display `(ast-3: ,ast-3)) (newline))
         ;; Remove redundancy.
         (ast-4 (ast-optimize-helper ast-3 bounds))
         ;;(_ (display `(ast-4: ,ast-4)) (newline))
         ;;(_  (newline))
         )
    ast-4))

;;; A convenience macro allowing to omit the bounds argument, taken as () when
;;; missing.
(define-macro (ast-optimize ast . optional-bounds)
  (unless (null-or-singleton? optional-bounds)
    (error `(ast-optimize: optional arguments ,optional-bounds
                           not null nor a singleton)))
  `(ast-optimize-procedure ,ast ,(car-or-nil optional-bounds)))

;;; Given a closure consistently alpha-convert it and return a list of
;;; three elements:
;;; - the new closure environment;
;;; - the new formals;
;;; - the new body.
(define-constant (closure-alpha-convert closure)
  ;; Bind the fields from the unoptimized closure.
  (let ((env (interpreted-closure-environment closure))
        (formals (interpreted-closure-formals closure))
        (body (interpreted-closure-body closure)))
    ;; Compute new fields.
    (let* ((unary-gensym (lambda (useless) (gensym)))
           (nonlocals (map car env))
           (fresh-formals (map unary-gensym formals))
           (fresh-nonlocals (map unary-gensym env))
           (alpha-converted-env (zip fresh-nonlocals (map cdr env)))
           (alpha-conversion-alist
            (append (zip formals fresh-formals)
                    (zip nonlocals fresh-nonlocals)))
           (alpha-converted-body
            (ast-alpha-convert-with body alpha-conversion-alist)))
      ;; Return the results.
      (list alpha-converted-env
            fresh-formals
            alpha-converted-body))))

;;; Destructively modify the given closure, consistently alpha-converting its
;;; nonlocals, formals and body.
(define-constant (closure-alpha-convert! closure)
  (let* ((fields (closure-alpha-convert closure))
         (alpha-converted-env (car fields))
         (alpha-converted-formals (cadr fields))
         (alpha-converted-body (caddr fields)))
    ;; Set all the fields, at the same time.  Doing this in more than one
    ;; operation would be dangerous as the closure we are updating might be
    ;; used in the update process itself, which would make visible fields in a
    ;; temporarily inconsistent state.
    (interpreted-closure-set! closure
                              alpha-converted-env
                              alpha-converted-formals
                              alpha-converted-body
                  )))

;;; Destructively modify the given closure, replacing its fields with a
;;; semantically equivalent optimized version.
(define-constant (interpreted-closure-optimize! closure)
  (when (compiled-closure? closure)
    (error `(cannot optimize the already compiled closure ,closure)))
  ;; First alpha-convert the closure; optimization might rely on this.
  (let* ((fields (closure-alpha-convert closure))
         (alpha-converted-env (car fields))
         (alpha-converted-formals (cadr fields))
         (alpha-converted-body (caddr fields)))
    ;; Set all the fields, at the same time, like in closure-alpha-convert! ;
    ;; but in this case use an optimized version of the body.
    (let* ((alpha-converted-bounds (set-unite alpha-converted-formals
                                              (map car alpha-converted-env)))
           (optimized-body (ast-optimize alpha-converted-body
                                         alpha-converted-bounds)))
      (interpreted-closure-set! closure
                                alpha-converted-env
                                alpha-converted-formals
                                optimized-body
                                ))))

;;; Destructively modify the given closure, replacing its fields with a
;;; semantically equivalent optimized version.  Return the argument.  This is a
;;; trivial convenience wrapper around interpreted-closure-optimize! , meant for
;;; interactive use.
(define-constant (interpreted-closure-optimize closure)
  (interpreted-closure-optimize! closure)
  closure)

;;; Another convenience wrapper for interactive use, also accepting (and
;;; currently doing nothing on) compiled closures.
(define-constant (closure-optimize closure)
  (unless (closure? closure)
    (error `(closure-optimized called on non-closure ,closure)))
  (when (interpreted-closure? closure)
    (interpreted-closure-optimize! closure))
  closure)




;;;; Retroactive optimization.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Optimize composed cons accessors.  Those are important for performance, and
;;; the rewriting itself, which should be fast.
(define-constant (optimize-cons-accessors-retroactively!)
  ;; I want to flatten composed cons accessors, making them all leaf procedures
  ;; each only using primitives and one variable.
  ;; First inline cons accessors of size 2, which will flatten them; then do the
  ;; same with cons accessors of size 3 (defined using accessors of size 2),
  ;; which will flatten them as well; then cons accessors of size 4.
  (let ((2-accessors (list caar cadr cdar cddr
                           set-caar! set-cadr! set-cdar! set-cddr!))
        (3-accessors (list caaar caadr cadar caddr
                           cdaar cdadr cddar cdddr
                           set-caaar! set-caadr! set-cadar! set-caddr!
                           set-cdaar! set-cdadr! set-cddar! set-cdddr!))
        (4-accessors (list caaaar caaadr caadar caaddr
                           cadaar cadadr caddar cadddr
                           cdaaar cdaadr cdadar cdaddr
                           cddaar cddadr cdddar cddddr
                           set-caaaar! set-caaadr! set-caadar! set-caaddr!
                           set-cadaar! set-cadadr! set-caddar! set-cadddr!
                           set-cdaaar! set-cdaadr! set-cdadar! set-cdaddr!
                           set-cddaar! set-cddadr! set-cdddar! set-cddddr!)))
    (dolist (accessor 2-accessors)
      (optimize-when-interpreted-closure! accessor))
    (dolist (accessor 3-accessors)
      (optimize-when-interpreted-closure! accessor))
    (dolist (accessor 4-accessors)
      (optimize-when-interpreted-closure! accessor))))

;;; Optimize every globally defined closure, constant or not, therefore
;;; retroactively optimizing the code defined up to this point.
;;; This is defined in a procedure to make it easy to disable, as the
;;; optimization process itself may be relatively inefficient.
(define-constant (optimize-global-closures-retroactively!)
  ;; AST rewriting will inline leaf calls, and therefore rewriting may turn a
  ;; non-leaf procedure into a leaf procedure, enabling more rewriting.  Doing
  ;; this systematically until no more leaf inlining is possible would require
  ;; a call graph, or some very inefficient alternative.
  ;; I accept this approximation: once the cons composed selectors are
  ;; flattened optimize *every* closure, just once, in an unspecified order.
  ;; Optimizations other than leaf inlining should not be affected by the
  ;; order.
  (dolist (symbol (interned-symbols))
    (when (and (defined? symbol)
               (closure? (symbol-global symbol)))
      (display `(optimizing ,symbol)) (newline)
      (optimize-when-interpreted-closure! (symbol-global symbol)))))

;;; Flatten composed cons accessors and optimize everything else once.
(define-constant (optimize-retroactively!)
  (optimize-cons-accessors-retroactively!)
  (optimize-global-closures-retroactively!))

;; Perform the retroactive rewriting.  This is the call to disable if a low
;; startup latency matters more than execution speed.
;;(optimize-retroactively!)




;;;; Variadic eval and macroexpand.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; FIXME: use null-or-singleton? and car-or-nil .  Possibly define a
;;; helper macro just for this case, which should be common.

(define-macro (eval form . optional-environment)
  (if (null? optional-environment)
      `(primordial-eval ,form ())
      `(primordial-eval ,form ,@optional-environment)))
(define-macro (eval-interpreter form . optional-environment)
  (if (null? optional-environment)
      `(primordial-eval-interpreter ,form ())
      `(primordial-eval-interpreter ,form ,@optional-environment)))
(define-macro (eval-vm form . optional-environment)
  (if (null? optional-environment)
      `(primordial-eval-vm ,form ())
      `(primordial-eval-vm ,form ,@optional-environment)))

(define-macro (macroexpand form . optional-environment)
  (if (null? optional-environment)
      `(primordial-macroexpand ,form ())
      `(primordial-macroexpand ,form ,@optional-environment)))




;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; Compiler.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;; Compiler utility code.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Given a set-as-list of fixnums, return the minimum non-negative fixnum not
;;; within the set.
(define-constant (compiler-smallest-not-in set)
  (let loop ((candidate 0))
    (if (set-has? set candidate)
        (loop (1+ candidate))
        candidate)))




;;;; Compiler state structure.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; The compiler state, conceptually a mutable record, is crudely implemented as
;;; a mutable list with each field in a fixed position.
;;; Notice that it's the list *elements* which are modifiable, and not the list
;;; *spine*.  This makes accessors a little nicer to write.
(define-constant (compiler-make-state)
  (list ()                          ;; instructions
        0                           ;; next-label
        ()                          ;; bindings
        ()                          ;; constant names being compiled (set)
        ;; Here begins the second part of the state (its cddddr).
        ()                          ;; closures being compiled (set)
        (compiler-flags-default)))  ;; flags

(define-constant (compiler-reversed-instructions state)
  (car state))
(define-constant (compiler-set-reversed-instructions! state new-field)
  (set-car! state new-field))

(define-constant (compiler-next-label state)
  (cadr state))
(define-constant (compiler-set-next-label! state new-field)
  (set-cadr! state new-field))

(define-constant (compiler-bindings state)
  (caddr state))
(define-constant (compiler-set-bindings! state new-field)
  (set-caddr! state new-field))

(define-constant (compiler-constant-names state)
  (cadddr state))
(define-constant (compiler-set-constant-names! state new-field)
  (set-cadddr! state new-field))

;;; Make it easier to access the second part of the list.
(define-constant (compiler-state-second-part state)
  (cddddr state))

(define-constant (compiler-closures state)
  (car (compiler-state-second-part state)))
(define-constant (compiler-set-closures! state new-field)
  (set-car! (compiler-state-second-part state) new-field))

(define-constant (compiler-flags state)
  (cadr (compiler-state-second-part state)))
(define-constant (compiler-set-flags! state new-field)
  (set-cadr! (compiler-state-second-part state) new-field))




;;;; Compiler state: generating instructions.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (compiler-add-instruction! state new-instruction)
  (let ((reversed-instructions (compiler-reversed-instructions state)))
    (compiler-set-reversed-instructions! state
                                         (cons new-instruction
                                               reversed-instructions))))




;;;; Compiler state: labels.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (compiler-new-label state)
  (let* ((old-count (compiler-next-label state)))
    (compiler-set-next-label! state (1+ old-count))
    old-count))

;;; Return a set-of-list of the labels occurring in the given compiler state.
(define-constant (compiler-used-labels state)
  (let* ((reversed-instructions (compiler-reversed-instructions state))
         (label-instructions (filter-reversed (lambda (i) (eq? (car i) 'label))
                                              reversed-instructions))
         (labels (map-reversed cadr label-instructions)))
    ;; A sanity check which should be very cheap.  FIXME: possibly remove.
    (unless (= (length labels) (length (list->set labels)))
      (error `(duplicate labels among ,labels)))
    labels))



;;;; Compiler state: bindings.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; The bindings field of the compiler state is an ordered list (the first
;;; binding takes precedence) of elements, each element holding information
;;; about where a variable or the closure environment is stored.
;;; If the compiled code is alpha-converted then the keys will be unique,
;;; but correctness doesn't rely on this.
;;;
;;; Each binding list element is a cons of one of the following
;;; two shapes:
;;; - (#t . REGISTER)
;;; - (VARIABLE . PLACE)
;;; where REGISTER is a non-negative fixnum (the index of register holding
;;; the closure environment), VARIABLE is a variable name as a symbol and
;;; PLACE has of one of the shapes:
;;; - (local-unboxed REGISTER)
;;; - (local-boxed REGISTER)
;;;      where REGISTER is a fixnum (the register index);
;;; - (nonlocal-unboxed INDEX)
;;; - (nonlocal-boxed INDEX)
;;;      where INDEX is a fixnum (the 0-based index of the matching value in
;;;      the closure environment).
;;; Globals are not stored in the bindings, but when looked up
;;; their PLACE looks like
;;; - global
;;;      , just a symbol.
;;; .

;;; Return non-#f iff the given place is a local place, either boxed or unboxed.
(define-constant (compiler-place-local? place)
  (and (cons? place)
       (or (eq? (car place) 'local-boxed)
           (eq? (car place) 'local-unboxed))))

;;; A "register place" is either a local or the non-local environment register.
;;; Return non-#f iff the given place is a register place.
(define-constant (compiler-place-register? place)
  (or (compiler-place-local? place)
      (fixnum? place)))

;;; Return non-#f iff the given place is a nonlocal place, either boxed or
;;; unboxed.
(define-constant (compiler-place-nonlocal? place)
  (and (cons? place)
       (or (eq? (car place) 'nonlocal-boxed)
           (eq? (car place) 'nonlocal-unboxed))))

;;; Return non-#f iff the given place is a global place.
(define-constant (compiler-place-global? place)
  (eq? place 'global))

;;; Given a non-global place as held in the bindings field of a compiler state,
;;; return the register index as a fixnum.
(define-constant (compiler-place->register place)
  (cond ((compiler-place-local? place)
         ;; Here place must have either the shape (local-unboxed REGISTER-INDEX)
         ;; or the shape (local-boxed REGISTER-INDEX) .
         (cadr place))
        ((fixnum? place)
         ;; This is the register environment place.
         place)
        ((eq? place 'global)
         ;; We don't have a register to return.
         (error '(compiler-place->register: globals not supported)))
        (else
         ;; Nonlocals don't have an associated register.
         (error '(compiler-place->register: place ,place supported)))))

;;; Return a fresh set-as-list of the register indices used in the given state.
(define-constant (compiler-used-registers state)
  (let* ((places (map cdr (compiler-bindings state)))
         (register-places (filter compiler-place-register? places))
         (register-list (map! compiler-place->register register-places)))
    (list->set register-list)))

(define-constant (compiler-fresh-register state)
  (let ((used-registers (compiler-used-registers state)))
    (compiler-smallest-not-in used-registers)))

;;; Return the register index for nonlocals, or error out if none is bound.
(define-constant (compiler-nonlocal-register state)
  (let ((cons-or-nil (assq #t (compiler-bindings state))))
    (if (null? cons-or-nil)
        (error '(compiler-nonlocal-register: no nonlocal in ,state))
        (cdr cons-or-nil))))

(define-constant (compiler-bound-variable? state variable)
  (assq variable (compiler-bindings state)))

(define-constant (compiler-lookup-variable state variable)
  (let ((cons-or-false (assq variable (compiler-bindings state))))
    (if cons-or-false
        (cdr cons-or-false)
        'global)))

(define-constant (compiler-bind! state variable-or-true place)
  (let ((bindings (compiler-bindings state)))
    (compiler-set-bindings! state
                            (cons (cons variable-or-true place)
                                  bindings))))

(define-constant (compiler-unbind! state variable-or-true)
  (let ((bindings (compiler-bindings state))
        (place (compiler-lookup-variable state variable-or-true)))
    (unless (compiler-place-local? place)
      ;; It only makes sense to unbind local-unboxed and local-boxed variables.
      (error `(cannot unbind ,variable-or-true from ,place)))
    (compiler-set-bindings! state
                            (del-assq-1-noncopying variable-or-true
                                                   bindings))))

(define-constant (compiler-bind-local-helper! state variable-name wrapper)
  (let* ((register (compiler-fresh-register state))
         (place (wrapper register)))
    (compiler-bind! state variable-name place)
    place))
(define-constant (compiler-bind-local-unboxed! state variable-name)
  (compiler-bind-local-helper! state
                               variable-name
                               (lambda (register) `(local-unboxed ,register))))
(define-constant (compiler-bind-local-boxed! state variable-name)
  (compiler-bind-local-helper! state
                               variable-name
                               (lambda (register) `(local-boxed ,register))))




;;;; Compiler state: known closures.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Add a closure to the set of closures being compiled in the given state.  It
;;; is harmless to add the same closure more than once.
(define-constant (compiler-add-closure! state closure)
  (let* ((old-set (compiler-closures state))
         (new-set (set-with old-set closure)))
    (compiler-set-closures! state new-set)))




;;;; Compiler state: flags.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; State flags are stored in an alist with unique keys and no default values.

;;; The initial flags for a fresh compiler state.
(define-constant compiler-flags-default-original
  '(;; The tail flag is non-#f iff the AST being compiled is in a tail
    ;; context.
    (tail . #t)
    ;; The used-result flag is non-#f iff the AST being compiled is in a
    ;; context where its result will be used; in particular it will be #f
    ;; when compiling the first form in a sequence.
    ;; Rationale: without using this flag I could always push a #<nothing>
    ;; literal at the end of the compilation of forms with unused results, with
    ;; the #<nothing> result being correctly dropped by a (drop) instruction
    ;; from the compilation of the sequence form.  However it's not always
    ;; possible to optimize away those trivial sequences by Jitter-level
    ;; rewriting, since the push-literal instruction might not occur immediately
    ;; before the drop instruction; that happens frequently with non-tail
    ;; conditionals in imperative code.
    (used-result . #t)))

;;; Return a fresh copy of the compiler default flags.  Since the alist is
;;; updated destructively it's important to avoid sharing, both in the spine and
;;; in the elements.
(define-constant (compiler-flags-default)
  (map-reversed (lambda (c) (cons (car c) (cdr c)))
                compiler-flags-default-original))

;;; Return the value associated to the given key.
(define-constant (compiler-flags-get flags name)
  (let ((cons-to-read (assq name flags)))
    (unless (cons? cons-to-read)
      (error `(compiler-flags-get: unbound flag ,name)))
    (cdr cons-to-read)))

;;; Destructivlely update the given flags, setting the given key to the given
;;; value.  Error out if the key is not already bound.
(define-constant (compiler-flags-set! flags name value)
  (let ((cons-to-update (assq name flags)))
    (unless (cons? cons-to-update)
      (error `(compiler-flags-set!: unbound flag ,name)))
    (set-cdr! cons-to-update value)))

;;; Convenience flag accessors.
(define-constant (compiler-flag state flag-name)
  (compiler-flags-get (compiler-flags state) flag-name))
(define-constant (compiler-set-flag! state flag-name flag-value)
  (compiler-flags-set! (compiler-flags state) flag-name flag-value))

;;; Convenience flag accessors for specific flags.
(define-constant (compiler-tail? state)
  (compiler-flag state 'tail))
(define-constant (compiler-set-tail! state value)
  (compiler-set-flag! state 'tail value))
(define-constant (compiler-used-result? state)
  (compiler-flag state 'used-result))
(define-constant (compiler-set-used-result! state value)
  (compiler-set-flag! state 'used-result value))

;;; Execute the given BODY-FORMS with the given compiler FLAG-NAME temporarily
;;; changed to FLAG-VALUE in the STATE , then reset FLAG-NAME to its original
;;; setting and return the result of the last form.
;;; FLAG-NAME must be a symbol, and is not evaluated.
;;; I guess this would be a good place to use dynamically-scoped variables,
;;; if they existed in JitterLisp.
(define-macro (compiler-with-flag state flag-name flag-value . body-forms)
  (let ((state-name (gensym))
        (outer-value-name (gensym))
        (flag-value-name (gensym))
        (result-name (gensym)))
    `(let* ((,state-name ,state)
            (,flag-value-name ,flag-value)
            (,outer-value-name (compiler-flag ,state-name ',flag-name)))
       (compiler-set-flag! ,state-name ',flag-name ,flag-value-name)
       (let ((,result-name ,@body-forms))
         (compiler-set-flag! ,state-name ',flag-name ,outer-value-name)
         ,result-name))))

;;; Temporarily flag changes for specific flags.
(define-macro (compiler-with-non-tail state . body-forms)
  `(compiler-with-flag ,state tail #f ,@body-forms))
(define-macro (compiler-with-used-result state . body-forms)
  `(compiler-with-flag ,state used-result #t ,@body-forms))
(define-macro (compiler-with-unused-result state . body-forms)
  `(compiler-with-flag ,state used-result #f ,@body-forms))




;;;; Stuff to move.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-constant (compiler-emit-return-when-tail! s)
  (when (compiler-tail? s)
    (compiler-add-instruction! s '(return))))

;;; Generate the appropriate instructions to bind the top of the stack to the
;;; given variable, as used free in the given body.  This procedure determines
;;; whether the variable needs boxing, no boxing, or no binding whatsoever.
;;; Return the place where variable-name is held, or the symbol nowhere .
(define-constant (compiler-pop-and-bind! s variable-name body)
  (cond ((not (ast-has-free? body variable-name))
         ;; The variable is not used.
         (compiler-add-instruction! s '(drop))
         'nowhere)
        ((ast-requires-boxing-for? body variable-name)
         ;; The variable is used and requires boxing.
         ;; (compiler-add-instruction! s '(box)) ;; FIXME: I believe this is wrong.  The thing is already boxed, and I don't need a second layer.
         (let ((place (compiler-bind-local-boxed! s variable-name)))
           (compiler-add-instruction! s `(pop-to-register ,(cadr place)))
           place))
        (else
         ;; The variable is used and doesn't require boxing.
         (let ((place (compiler-bind-local-unboxed! s variable-name)))
           (compiler-add-instruction! s `(pop-to-register ,(cadr place)))
           place))))

;; FIXME: factor the compilation of lambda and of existing closures into
;; this, if indeed there is anything to factor.
(define-constant (compiler-bind-nonlocals! s ??? formals body)
  (error '(unimplemented: compiler-bind-nonlocals!)))

;;; Remove actuals from the stack and bind them to the given formals (here given
;;; in the order in which they occur in a lambda, which is the evaluation order
;;; -- to be popped right-to-left), simply dropping the ones which are not used
;;; in the body.  Also drop the closure, found on the stack below the actuals,
;;; and bind nonlocals if needed.
(define-constant (compiler-pop-args-and-closure! s formals body)
  ;; I have two different implementations of this.
  (compiler-pop-args-and-closure!-one-by-one s formals body)
  ;;(compiler-pop-args-and-closure!-one-shot s formals body)
  )

;;; One implementation of compiler-pop-args-and-closure! .  The strategy of this
;;; implementation is loading every useful element to registers without
;;; affecting the stack, and then dropping everything in one shot.
;;; Only one of the implementations is used, but I'm keeping both as I am still
;;; undecided about the merits of each.
(define-constant (compiler-pop-args-and-closure!-one-shot s formals body)
  (let ((formal-no (length formals))
        (depth 0)) ;; The depth for the current argument or closure.
    (dolist (formal (reverse formals))
      (cond ((not (ast-has-free? body formal))
             ;; The variable is not used.  Do nothing.
             )
            ((ast-requires-boxing-for? body formal)
             ;; The variable is used and requires boxing.
             (compiler-non-dropping-bind-at-depth! s formal depth #t))
            (else
             ;; The variable is used and doesn't require boxing.
             (compiler-non-dropping-bind-at-depth! s formal depth #f)))
      (set! depth (1+ depth)))
    ;; The called closure is at depth depth.  Bind the nonlocal environment from
    ;; the closure, if needed.
    (when (exists? (lambda (binding) (compiler-place-nonlocal? (cdr binding)))
                   (compiler-bindings s))
      (let ((register (compiler-fresh-register s)))
        (compiler-bind! s #t register)
        (if (zero? depth)
            (compiler-add-instruction! s `(copy-to-register ,register))
            (compiler-add-instruction! s `(at-depth-to-register ,depth ,register)))
        (compiler-add-instruction! s `(dereference-nonlocals ,register))))
    ;; Drop all the actuals plus the closure.
    (dotimes (i (+ formal-no 1))
      (compiler-add-instruction! s '(drop)))))

;;; An alternative implementation of compiler-pop-args-and-closure!, simpler but
;;; generating worse code when used and unused arguments are mixed.  On the
;;; other hand the VM instructions generated by this implementation are easier
;;; to rewrite.
;;; A good test case to show the difference:
;;;   (define (f a b c d e f g) (+ a c e g))
;;; Only one of the implementations is used, but I'm keeping both as I am still
;;; undecided about the merits of each.
(define-constant (compiler-pop-args-and-closure!-one-by-one s formals body)
  (dolist (formal (reverse formals))
    (compiler-pop-and-bind! s formal body))
  ;; Now the top of the stack contains the called closure.  Bind the nonlocal
  ;; environment from the closure, if needed; in either case drop the top
  ;; element.
  (when (exists? (lambda (binding) (compiler-place-nonlocal? (cdr binding)))
                 (compiler-bindings s))
    (let ((register (compiler-fresh-register s)))
      (compiler-bind! s #t register)
      (compiler-add-instruction! s `(nonlocals-to-register ,register))))
  (compiler-add-instruction! s '(drop)))

;; Generate an instruction for the given compiler state loading the named
;; variable from the stack at the given depth (depth 0 means top), without
;; destructively updating the stack; the variable will be boxed if boxed is
;; non-#f.  Update the state with the new binding.
(define-constant (compiler-non-dropping-bind-at-depth! s name depth boxed)
  ;; Determine in which register to hold the variable binding.
  (let* ((place (if boxed
                    (compiler-bind-local-boxed! s name)
                    (compiler-bind-local-unboxed! s name)))
         (register (cadr place)))
    ;; Generate an instruction loading the register, without modifying the stack.
    (if (zero? depth)
        (compiler-add-instruction! s `(copy-to-register ,register))
        (compiler-add-instruction! s `(at-depth-to-register ,depth ,register)))))

;;; Generate an instruction pushing a #<nothing> literal.
(define-constant (compiler-push-nothing! s)
  (compiler-add-instruction! s `(push-literal ,(begin))))

;;; Generate instructions saving the contents of the given set-of-list of
;;; registers as fixnums, in a conventional order.
(define-constant (compiler-save-registers! s registers)
  (dolist (register (sort registers))
    (compiler-add-instruction! s `(save-register ,register))))

;;; Generate instructions saving the contents of the given set-of-list of
;;; registers, in a conventional order compatible with (which is to say,
;;; opposite to) the instructions generated by compiler-save-registers! .
(define-constant (compiler-restore-registers! s registers)
  (dolist (register (reverse (sort registers)))
    (compiler-add-instruction! s `(restore-register ,register))))




;;;; Compiling an AST.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Compile the given AST in the given state.  This works by calling the
;;; appropriate helper according to the AST case.
(define-constant (compile-ast! s ast)
  (cond ((ast-literal? ast)
         (compile-literal! s (ast-literal-value ast)))
        ((ast-variable? ast)
         (compile-variable! s (ast-variable-name ast)))
        ((ast-define? ast)
         (compile-define s
                         (ast-define-name ast)
                         (ast-define-body ast)))
        ((ast-if? ast)
         (compile-if! s
                      (ast-if-condition ast)
                      (ast-if-then ast)
                      (ast-if-else ast)))
        ((ast-set!? ast)
         (compile-set!! s
                        (ast-set!-name ast)
                        (ast-set!-body ast)))
        ((ast-while? ast)
         (compile-while! s
                        (ast-while-guard ast)
                        (ast-while-body ast)))
        ((ast-primitive? ast)
         (compile-primitive! s
                            (ast-primitive-operator ast)
                            (ast-primitive-operands ast)))
        ((ast-call? ast)
         (compile-call! s
                       (ast-call-operator ast)
                       (ast-call-operands ast)))
        ((ast-lambda? ast)
         (compile-lambda! s
                         (ast-lambda-formals ast)
                         (ast-lambda-body ast)))
        ((ast-let? ast)
         (compile-let! s
                      (ast-let-bound-name ast)
                      (ast-let-bound-form ast)
                      (ast-let-body ast)))
        ((ast-sequence? ast)
         (compile-sequence! s
                           (ast-sequence-first ast)
                           (ast-sequence-second ast)))))

(define-constant (compile-literal! s value)
  (when (compiler-used-result? s)
    (compiler-add-instruction! s `(push-literal ,value))
    (compiler-emit-return-when-tail! s)))

(define-constant (compile-variable! s name)
  (let ((place (compiler-lookup-variable s name)))
    (cond ((eq? place 'global)
           (if (constant? name)
               (let ((value (symbol-global name)))
                 (when (compiler-used-result? s)
                   (compiler-add-instruction! s `(push-literal ,value))))
               (if (compiler-used-result? s)
                   (compiler-add-instruction! s `(push-global ,name))
                   (compiler-add-instruction! s `(check-global-defined ,name)))))
          ((compiler-place-local? place)
           (when (compiler-used-result? s)
             (compiler-add-instruction! s `(push-register ,(cadr place)))
             (when (eq? (car place) 'local-boxed)
               (compiler-add-instruction! s `(unbox)))))
          ((compiler-place-nonlocal? place)
           (when (compiler-used-result? s)
             (let ((nonlocal-register (compiler-nonlocal-register s))
                   (nonlocal-index (cadr place)))
               (compiler-add-instruction! s `(push-register ,nonlocal-register))
               (compiler-add-instruction! s `(nonlocal ,nonlocal-index))
               (when (eq? (car place) 'nonlocal-boxed)
                 (compiler-add-instruction! s `(unbox))))))
          (else
           (error `(unimplemented variable place ,place)))))
  (compiler-emit-return-when-tail! s))

(define-constant (compile-define s name body)
  (compiler-with-non-tail s
    (compiler-with-used-result s
      (compile-ast! s body)))
  (compiler-add-instruction! s `(pop-to-global ,name))
  (when (compiler-used-result? s)
    (compiler-push-nothing! s))
  (compiler-emit-return-when-tail! s))

(define-constant (compile-if! s condition then else)
  (let ((after-then-label (compiler-new-label s))
        (after-else-label (compiler-new-label s)))
    (compiler-with-non-tail s
      (compiler-with-used-result s
        (compile-ast! s condition)))
    (compiler-add-instruction! s `(branch-if-false ,after-then-label))
    (compile-ast! s then)
    (unless (compiler-tail? s)
      (compiler-add-instruction! s `(branch ,after-else-label)))
    (compiler-add-instruction! s `(label ,after-then-label))
    (compile-ast! s else)
    (unless (compiler-tail? s)
      (compiler-add-instruction! s `(label ,after-else-label)))))

(define-constant (compile-set!! s name body)
  (let ((place (compiler-lookup-variable s name)))
    (compiler-with-non-tail s
      (compiler-with-used-result s
        (compile-ast! s body)))
    (cond ((compiler-place-global? place)
           (compiler-add-instruction! s `(pop-to-global-defined ,name)))
          ((eq? (car place) 'local-unboxed)
           (compiler-add-instruction! s `(pop-to-register ,(cadr place))))
          ((eq? (car place) 'local-boxed)
           (compiler-add-instruction! s `(pop-to-register-box ,(cadr place))))
          ((eq? (car place) 'nonlocal-unboxed)
           (error `(compile-set!!: ,name is nonlocal-unboxed but is assigned)))
          ((eq? (car place) 'nonlocal-boxed)
           (let ((nonlocal-register (compiler-nonlocal-register s))
                 (index (cadr place)))
             (compiler-add-instruction! s `(push-register ,nonlocal-register))
             (compiler-add-instruction! s `(set-nonlocal ,index))))
          (else
           (error `(unsupported set! place ,place)))))
  (when (compiler-used-result? s)
    (compiler-push-nothing! s))
  (compiler-emit-return-when-tail! s))

(define-constant (compile-infinite-loop! s body)
  (let ((before-body-label (compiler-new-label s)))
    (compiler-add-instruction! s `(label ,before-body-label))
    (compiler-with-non-tail s
      (compiler-with-unused-result s
        (compile-ast! s body)))
    (compiler-add-instruction! s `(branch ,before-body-label))
    ;; At this point I would normally emit one instruction to push #<nothing>
    ;; when the result is used, and a return when the context is tail; but in
    ;; this case they would be unreachable after an infinite loop.
    ))

(define-constant (compile-while-ordinary! s guard body)
  (let ((before-body-label (compiler-new-label s))
        (before-guard-label (compiler-new-label s)))
    (compiler-add-instruction! s `(branch ,before-guard-label))
    (compiler-add-instruction! s `(label ,before-body-label))
    (compiler-with-non-tail s
      (compiler-with-unused-result s
        (compile-ast! s body)))
    (compiler-add-instruction! s `(label ,before-guard-label))
    (compiler-with-non-tail s
      (compiler-with-used-result s
        (compile-ast! s guard)))
    (compiler-add-instruction! s `(branch-if-true ,before-body-label)))
  (when (compiler-used-result? s)
    (compiler-push-nothing! s))
  (compiler-emit-return-when-tail! s))

(define-constant (compile-while! s guard body)
  (if (and (ast-literal? guard)
           (ast-literal-value guard))
      (compile-infinite-loop! s body)
      (compile-while-ordinary! s guard body)))

(define-constant (compile-primitive! s operator operands)
  (dolist (operand operands)
    (compiler-with-non-tail s
      (compiler-with-used-result s
        (compile-ast! s operand))))
  (compiler-add-instruction! s `(primitive ,operator))
  (unless (compiler-used-result? s)
    (compiler-add-instruction! s '(drop)))
  (compiler-emit-return-when-tail! s))

(define-constant (compile-call! s operator operands)
  (let* ((literal-closure-operator
          ;; Non-#f iff the operator is a literal closure.
          (and (ast-literal? operator)
               (closure? (ast-literal-value operator))))
         (global-constant-operator
          ;; Non-#f iff the operator is a non-shadowed global bound to a
          ;; constant.
          (and (ast-variable? operator)
               (not (compiler-bound-variable? s (ast-variable-name operator)))
               (constant? (ast-variable-name operator))))
         (known-closure
          ;; Bind known-closure to the called closure if I can resolve it at
          ;; this time, otherwise to #f.
          (cond (literal-closure-operator
                 ;; The operator is a literal closure.
                 (ast-literal-value operator))
                (global-constant-operator
                 ;; The operator is a variable globally bound to a constant
                 ;; and not shadowed.
                 (symbol-global (ast-variable-name operator)))
                (else
                 ;; The closure is not known: I can't omit run-time checks.
                 #f)))
         (known-compiled
          ;; Non-#f iff the closure is known to be compiled.  This can happen
          ;; as long as we know the operator to be a closure, in three cases:
          (and known-closure
               (or ;; (a) I already know what the compiled closure is...
                   (compiled-closure? known-closure)
                   ;; (b) I know that the operator is the unshadowed global constant
                   ;;     name of a global being compiled.
                   (and global-constant-operator
                        (set-has? (compiler-constant-names s)
                                  (ast-variable-name operator)))
                   ;; (c) The operator is a literal interpreted closure which is
                   ;;     being compiled now, and therefore counts as if it were
                   ;;     already compiled when called.
                   (and literal-closure-operator
                        (set-has? (compiler-closures s)
                                  (ast-literal-value operator))))))
         (tail (compiler-tail? s))
         (call-instruction
          ;; The VM instruction to use for calling.
          (cond ((and known-compiled tail)
                 'tail-call-compiled)
                (tail
                 'tail-call)
                (known-compiled
                 'call-compiled)
                (else
                 'call))))
    (compiler-with-non-tail s
      (compiler-with-used-result s
        (compile-ast! s operator)))
    (unless known-closure
      (compiler-add-instruction! s `(check-closure)))
    (unless (and known-closure
                 (= (closure-in-arity known-closure) (length operands)))
      (when known-closure
        ;; FIXME: warn more cleanly.
        (display `(WARNING: in-arity mismatch in call to known closure
                            ,operator with actuals ,operands))
        (newline))
      (compiler-add-instruction! s `(check-in-arity ,(length operands))))
    (dolist (operand operands)
      (compiler-with-non-tail s
                              (compiler-with-used-result s
                                                         (compile-ast! s operand))))
    (if (compiler-tail? s)
        (compiler-add-instruction! s `(,call-instruction ,(length operands)))
        (let ((used-registers (compiler-used-registers s)))
          ;; The registers being used at this point are a superset of the ones
          ;; live at return.  This is a conservative approximation, crude but
          ;; correct.  Doing better would require a liveness analysis pass.
          (compiler-save-registers! s used-registers)
          (compiler-add-instruction! s `(,call-instruction ,(length operands)))
          (compiler-restore-registers! s used-registers)
          (unless (compiler-used-result? s)
            (compiler-add-instruction! s '(drop)))))))

(define-constant (compile-let! s bound-name bound-form body)
  (compiler-with-non-tail s
    (compiler-with-used-result s
      (compile-ast! s bound-form)))
  (let ((place (compiler-pop-and-bind! s bound-name body)))
    (compile-ast! s body)
    (unless (eq? place 'nowhere)
      (compiler-unbind! s bound-name))))

(define-constant (compile-lambda! s formals body)
  (when (compiler-used-result? s)
    (error '(unimplemented: compile-lambda!))
    (compiler-emit-return-when-tail! s)))

(define-constant (compile-sequence! s first second)
  (compiler-with-non-tail s
    (compiler-with-unused-result s
      (compile-ast! s first)))
  (compile-ast! s second))




;;;; Compiling an interpreted closure.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Compiling an interpreted closure destructively changes it into a compiled
;;; closure, without affecting its identity.  The change is irreversible: an
;;; interpreted closure can become compiled, but it's not possible to uncompile
;;; a compiled closure.

;;; Compile the given closure, knowing the that given set-as-list of symbols are
;;; global names of constant closures being compiled, possibly including c ,
;;; and that the given set-of-list of closures are also being compiled (again,
;;; possibly including the global value of c).
;;; Rationale: independently from the order of compilation, the compiled will be
;;; able to assume that any (free) call from c to one of the named procedures
;;; will be to a compiled procedure: this saves a conditional per call at
;;; execution time.
;;; It would be possible to do even better by compiling every closure in one go,
;;; and directly refer compiled code instead of symbols.
(define-constant (interpreted-closure-compile!-knowing
                     c
                     constant-names-being-compiled
                     closures-being-compiled)
  ;; Validate arguments.
  (when (compiled-closure? c)
    (error `(closure ,c is already compiled)))
  (dolist (name constant-names-being-compiled)
    (unless (symbol? name)
      (error `(name for constant closure being compiled not a symbol: ,name)))
    (unless (defined? name)
      (error `(name for constant closure being compiled not globally bound:
                    ,name)))
    (unless (constant? name)
      (error `(named constant closure being compiled not constant: ,name)))
    (unless (interpreted-closure? (symbol-global name))
      (error `(named constant closure being compiled not an interpreted
                    closure: ,name))))
  (let ((env (interpreted-closure-environment c))
        (formals (interpreted-closure-formals c))
        (body (interpreted-closure-body c)))
    (let ((s (compiler-make-state))
          (next-nonlocal-index 0)
          (bound-nonlocal-names set-empty)
          (reversed-nonlocal-values ()))
      ;; Record the names of the constant closures being compiled, and their
      ;; values.
      (compiler-set-constant-names! s constant-names-being-compiled)
      (dolist (name constant-names-being-compiled)
        (compiler-add-closure! s (symbol-global name)))
      (dolist (closure closures-being-compiled)
              (compiler-add-closure! s closure))
      ;; Of course we are compiling the current closures.  Adding it to the
      ;; set of known-to-be-compiled closures will let the compiler generate
      ;; better code for recursive calls via literal operators.
      (compiler-add-closure! s c)
      ;; Emit the procedure prolog.
      (compiler-add-instruction! s '(procedure-prolog))
      ;; Bind every nonlocal which is not shadowed by a formal and which is
      ;; actually used.
      ;; Since here we are compiling an existing interpreted closure which used
      ;; boxing for every nonlocal we will use boxing in the compiled version as
      ;; well: there is no way of knowing which nonlocal actually needs boxing
      ;; to be shared correctly with other code.
      ;; Ignore non-local occurrences of already bound non-locals: only the
      ;; first binding counts for each variable, since what we find first
      ;; has been bound in the innermost context.
      (dolist (env-binding env)
        (when (and (not (set-has? formals (car env-binding)))
                   (not (set-has? bound-nonlocal-names (car env-binding)))
                   (ast-has-free? body (car env-binding)))
          (compiler-bind! s
                          (car env-binding)
                          `(nonlocal-boxed ,next-nonlocal-index))
          (set! next-nonlocal-index (+ next-nonlocal-index 1))
          (set! reversed-nonlocal-values
                (cons (cdr env-binding)
                      reversed-nonlocal-values))))
      (cond ((closure-primitive-wrapper? c)
             ;; Special case: we are compiling a primitive wrapper.
             (let ((primitive (ast-primitive-operator body)))
               ;; We can compile primitive wrappers (see the definition above)
               ;; in a more efficient way: primitive, nip, return.
               (compiler-add-instruction! s `(primitive ,primitive))
               (compiler-add-instruction! s '(nip))
               (compiler-add-instruction! s '(return))))
            (else
             ;; General case.
             ;; Generate code binding the formals we use and the nonlocals in
             ;; registers, ignoring the others and dropping them all, including
             ;; the closure argument.
             (compiler-pop-args-and-closure! s formals body)
             ;; Now the state contains bindings for every non-global bound in
             ;; the body AST, and we can compile it.  Any remaining variable
             ;; occurring in the body but not in the state bindings is a global.
             (compile-ast! s body)))
      ;;(print-compiler-state s)
      ;; We can now destructively modify the interpreted closure.
      (interpreted-closure-make-compiled!
          c
          (length formals)
          (reverse! reversed-nonlocal-values)
          (reverse! (compiler-reversed-instructions s)))
;;(newline) (disassemble-procedure c) (newline)
      )))

;;; A convenient procedure to call when one closure is being compiled in
;;; isolation.
(define-constant (interpreted-closure-compile!-procedure c)
  (interpreted-closure-compile!-knowing c () ()))
(define-macro (interpreted-closure-compile! . closures)
  `(map-syntactically interpreted-closure-compile!-procedure ,@closures))




;;;; Compiliation convenience procedures.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; It's nice to provide the user with procedures working indifferently on
;;; interpreted or compiled closures, compiling the intepreted ones on the fly
;;; if needed.

;;; This factors the common functionality of the user procedures below.
(define-constant (compile!-if-needed-then-call thing procedure)
  (cond ((compiled-closure? thing)
         ;; The thing is already compiled.
         (procedure thing))
        ((interpreted-closure? thing)
         ;; The thing is not compiled yet.  Optimize it and compile it, then do
         ;; the job.
         (interpreted-closure-optimize! thing)
         (interpreted-closure-compile! thing)
         (procedure thing))
        ((symbol? thing)
         ;; This is a name, hopefully for a closure.
         (let ((names (if (and (defined? thing)
                               (constant? thing))
                          (list thing)
                          ()))
               (c (symbol-global thing)))
           ;; Optimize the closure and compile it; if it's a global constant
           ;; we can use its name during compilation, to compile recursive
           ;; calls more efficiently.
           (interpreted-closure-optimize! c)
           (interpreted-closure-compile!-knowing c names ())))
        ((macro? thing)
         ;; Macros are only interpreted.  This is a current limitation that
         ;; could be lifted.
         (error `(macros not currently compilable: ,thing)))
        (else
         ;; This is a non-closure non-macro.
         (error `(cannot compile ,thing)))))

;;; Compile the given closure if needed.  Return nothing.
(define-constant (compile!-procedure thing)
  (compile!-if-needed-then-call thing (lambda (unused))))
(define-macro (compile! . things)
  `(map-syntactically compile!-procedure ,@things))

;; FIXME: generalize the procedures above to compiling a list or set of closures
;; all at the same time.  This will make inter-calls more efficient.

;;; Print a native-code disassembly of the given closure, compiling it first if
;;; needed.  Return nothing.
(define-constant (disassemble-procedure thing)
  (compile!-if-needed-then-call thing compiled-closure-disassemble))
(define-macro (disassemble . things)
  `(map-syntactically disassemble-procedure ,@things))

;;; Print VM code for the given closure, compiling it first if needed.  Return
;;; nothing.
(define-constant (disassemble-vm-procedure thing)
  (compile!-if-needed-then-call thing compiled-closure-print))
(define-macro (disassemble-vm . things)
  `(map-syntactically disassemble-vm-procedure ,@things))




;;;; Compiler scratch code.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Scratch, for debugging.
(define-constant (print-reversed-instructions xs)
  (dolist (x (reverse xs))
    (when (and (list? x)
               (not-eq? (car x) 'label))
      (dotimes (i 4)
        (character-display #\space)))
    (display x)
    (newline)))

;;; A debugging procedure.
(define-constant (print-compiler-state s)
  (display `(USED LABELS: ,@(sort (compiler-used-labels s))))
  (newline)
  (print-reversed-instructions (compiler-reversed-instructions s)))

;; Temporary testing procedure: unoptimized version.
(define (tup ast)
  (let ((s (compiler-make-state)))
    (display 'ast:) (dotimes (i 1) (character-display #\space))
    (display ast)
    (newline)
    (compile-ast! s ast)
    (print-compiler-state s)
    (newline)))

;; Temporary testing procedure: optimized version.
(define (top ast)
  (let ((s (compiler-make-state))
        (optimized-ast (ast-optimize ast ())))
    (display 'original:) (dotimes (i 2) (character-display #\space))
    (display ast)
    (newline)
    (display 'rewritten:) (dotimes (i 1) (character-display #\space))
    (display optimized-ast)
    (newline)
    (compile-ast! s optimized-ast)
    (print-compiler-state s)
    (newline)))

;; Temporary testing macro: unoptimized.
(define-macro (tu . forms)
  `(tup (macroexpand '(begin ,@forms))))

;; Temporary testing macro: optimized.
(define-macro (to . forms)
  `(top (macroexpand '(begin ,@forms))))




;;;; Implicit optimization: definitions.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; From now on definition forms will automatically optimize new globally bound
;;; closures.

;;; Keep the previous (macro) values for define and define-constant forms .
(define define-non-optimized
  define)
(define define-constant-non-optimized
  define-constant)

;;; Destructively optimize the argument if it's an interpreted closure; do
;;; nothing otherwise.  Return nothing.
(define-constant (optimize-when-interpreted-closure! thing)
  (when (interpreted-closure? thing)
    (interpreted-closure-optimize! thing)))

;;; Define the named thing, and immediately optimize it if it's a closure.  Same
;;; syntax as define.
(define-macro (define-optimized-possibly-constant constant thing . body)
  (cond ((symbol? thing)
         (let ((value-name (gensym)))
           `(let ((,value-name ,@body))
              (define-non-optimized ,thing ,value-name)
              (when ,constant
                ;; Making a globally defined closure a constant *before*
                ;; optimizing it may help the rewrite system.
                (make-constant! ',thing))
              (optimize-when-interpreted-closure! ,value-name))))
        ((or (not (symbols? thing))
             (not (all-different? (cdr thing))))
         (error `(define-optimized-possibly-constant: ill-formed defined
                   thing ,thing)))
        (else
         (let ((value-name (gensym))
               (thing-name (car thing))
               (thing-formals (cdr thing)))
           `(let ((,value-name (lambda ,thing-formals
                                 ,@body)))
              (define-non-optimized ,thing-name ,value-name)
              (when ,constant
                ;; Again, make the thing constant before optimizing it.
                (make-constant! ',thing-name))
              (optimize-when-interpreted-closure! ,value-name))))))
(define-macro (define-optimized thing . body)
  `(define-optimized-possibly-constant #f ,thing ,@body))
(define-macro (define-constant-optimized thing . body)
  `(define-optimized-possibly-constant #t ,thing ,@body))

;; Re-define define and define-constant to make them implicitly optimizing.
(define-macro (define thing . body)
  `(define-optimized ,thing ,@body))
(define-macro (define-constant thing . body)
  `(define-constant-optimized ,thing ,@body))




;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; The library is now loaded.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; This is checked at startup to error out in case the library is being loaded
;;; more than once.  Make the defined symbol constant, so that it can't be
;;; undefined.
(define-constant jitterlisp-library-loaded
  #t)




;;;; Tentative: variadic map.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; This macro factors the common code for the variadic versions of map-reversed
;;; and map! .  List reversal is not performed here, so the variadic version of
;;; map will use the variadic version of map-reversed , and then tail-call
;;; reverse! to destructively update the temporary list and turn it into the
;;; result.  map! needs no reversal.
;;;
;;; operator is "applied" in the sense of being copied into a list car in the
;;; expansion, independently from its type.  The exansion of this macro does not
;;; evaluate it.  Rationale: user macros relying on this one will evaluate the
;;; user operator and check its type: if the given operator is a macro they will
;;; call this macro with the operator name; otherwise, assuming the operator is
;;; a closure, they will pass the closure value.
(define-macro (vmap-internal destructive operator . lists)
  (unless (list? lists)
    (error `(vmap-internal: non-list lists argument: ,lists)))
  (unless (non-null? lists)
    (error `(vmap-internal: zero lists given)))
  (let* ((list-no (length lists))
         (list-names (map (lambda (whatever) (gensym))
                          lists))
         (first-list-name (car list-names))
         (result-name (gensym)))
    `(let* (,@(map (lambda (name-list) (list (car name-list) (cdr name-list)))
                   (zip list-names lists))
            (,result-name ,(if destructive
                               first-list-name
                               ())))
       (while (non-null? ,first-list-name)
         ,(if destructive
              `(set-car! ,first-list-name
                         (,operator ,@(map (lambda (list-name) `(car ,list-name))
                                           list-names)))
              `(set! ,result-name
                     (cons (,operator ,@(map (lambda (list-name) `(car ,list-name))
                                             list-names))
                           ,result-name)))
         ,@(map (lambda (list-name) `(set! ,list-name (cdr ,list-name)))
                list-names))
       ,@(map (lambda (list-name)
                `(unless (null? ,list-name)
                   (error '(vmap-internal: first list shorter than at least other one))))
              (cdr list-names))
       ,result-name)))

;;; FIXME: describe these in comments and document them in the manual.  They are easy.

(define-macro (vmap-reversed operator . lists)
  (let ((operator-value operator))
    (if (macro? operator-value)
        `(vmap-internal #f ,operator ,@lists)
        `(vmap-internal #f ,operator-value ,@lists))))

(define-macro (vmap operator . lists)
  `(reverse! (vmap-reversed ,operator ,@lists)))

(define-macro (vmap! operator . lists)
  (let ((operator-value operator))
    (if (macro? operator-value)
        `(vmap-internal #t ,operator ,@lists)
        `(vmap-internal #t ,operator-value ,@lists))))




;;;; Scratch.
;;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-macro (c lambda)
  (let ((closure-name (gensym)))
    `(let ((,closure-name ,lambda))
       (interpreted-closure-optimize! ,closure-name)
       (interpreted-closure-compile! ,closure-name)
       (newline)
       (compiled-closure-print ,closure-name)
       (newline)
       (compiled-closure-disassemble ,closure-name))))

;;; This is useful to test run-time type checking, particularly in compiled
;;; code.
(define-constant (count-i-incorrect a)
  (while (not (zero? a))
    (set! a (- a 'b)))
  a)

(define-constant (average-procedure a b)
  (/ (+ a b) 2))

(define-macro (average . things)
  (when (zero? (length things))
    (error '(average: zero arguments)))
  `(/ (+ ,@things)
      ,(length things)))

(define (sum xs)
  (let ((res 0))
    (while (not (null? xs))
      (set! res (+ res (car xs)))
      (set! xs (cdr xs))) res))

(define (qq n)
  (length (flatten (map iota (iota n)))))

(define-constant (make-tree depth)
  (if (zero? depth)
      ()
      (let ((branch (make-tree (- depth 1))))
        (cons branch branch))))

(define-constant (complexity-non-tail-recursive thing)
  (if (cons? thing)
      (+ 1
         (complexity-non-tail-recursive (car thing))
         (complexity-non-tail-recursive (cdr thing)))
      1))

(define-constant (complexity-tail-recursive thing)
  (complexity-tail-recursive-helper (list thing) 0))
(define-constant (complexity-tail-recursive-helper worklist acc)
  (cond ((null? worklist)
         acc)
        ((cons? (car worklist))
         (let ((first (car worklist)))
           (complexity-tail-recursive-helper (cons (car first)
                                                   (cons (cdr first)
                                                         (cdr worklist)))
                                             (+ acc 1))))
        (else
         (complexity-tail-recursive-helper (cdr worklist) (+ acc 1)))))

(define-constant (complexity-iterative thing)
  (let ((res 0)
        (worklist (list thing)))
    (while (not (null? worklist))
      (let ((first (car worklist)))
        (set! res (+ res 1))
        (if (cons? first)
            (begin
              (set-car! worklist (cdr first))
              (set! worklist (cons (car first) worklist)))
            (set! worklist (cdr worklist)))))
    res))

(define-constant (even?-tr n)
  (cond ((zero? n)
         #t)
        ((= n 1)
         #f)
        (else
         (odd?-tr (- n 1)))))
(define-constant (odd?-tr n)
  (cond ((zero? n)
         #f)
        ((= n 1)
         #t)
        (else
         (even?-tr (- n 1)))))


(define-constant (sign n)
  (cond ((zero? n) 0)
        ((< n 0)   -1)
        (else      1)))

(define (interpreted-closure-names)
  (filter (lambda (s)
            (and (defined? s)
                 (interpreted-closure? (symbol-global s))))
          (interned-symbols)))

(define (compile-all-sequentially!)
  (dolist (s (interpreted-closure-names))
    (display `(compiling ,s))
    (newline)
    (compile! (symbol-global s))))

(define-macro (compile-all!)
  (let ((interpreted-closure-names (interpreted-closure-names)))
    `(begin
       (display '(compiling ,(length interpreted-closure-names) procedures in one shot))
       (newline)
       (compile! ,@interpreted-closure-names))))

(define-constant (prime? n)
  (if (< n 2)
      #f
      (let ((i 2)
            (divisor-found #f))
        (while (and (not divisor-found)
                    (< i n))
               (when (zero? (remainder n i))
                 (set! divisor-found #t))
               (set! i (+ i 1)))
        (not divisor-found))))

(define-constant (primes-naif limit)
  (let ((reversed-res ()))
    (dotimes (i limit)
      (when (prime? i)
        (set! reversed-res (cons i reversed-res))))
    (reverse! reversed-res)))

(define-constant (prime?-knowing known-primes n)
  (if (< n 2)
      #f
      (let ((i 2)
            (remaining-candidate-factors known-primes)
            (divisor-found #f))
        (while (and (not divisor-found)
                    (not (null? remaining-candidate-factors)))
               (when (zero? (remainder n (car remaining-candidate-factors)))
                 (set! divisor-found #t))
               (set! remaining-candidate-factors
                     (cdr remaining-candidate-factors)))
        (not divisor-found))))

(define-constant (primes limit)
  (let ((known-primes ()))
    (dotimes (i limit)
      (when (prime?-knowing known-primes i)
        (set! known-primes (cons i known-primes))))
    (reverse! known-primes)))

;;; FIXME: these are good for testing return with defective instructions.
(define-constant (fibo-tr next-to-last last n) (if (zero? n) next-to-last (fibo-tr last (+ next-to-last last) (- n 1))))
(define-constant (fibof n) (fibo-tr 0 1 n))
(define-constant (fibor n) (if (< n 2) n (+ (fibor (- n 2)) (fibor (- n 1)))))

(define-macro (compiled-lambda formals . body)
  (let ((closure-name (gensym)))
    `(let ((,closure-name (lambda ,formals ,@body)))
       (compile! ,closure-name)
       ,closure-name)))

;;; I CAN DO BETTER IN THIS CASE:
;;; (lambda (f x) (f (begin y x)))
;;; Getting this right in the general case (any number of arguments, both
;;; procedures and primitives) would improve the quality of inlined code.
;;; The first expression in the sequence, here y, is allowed to have effects.

;;;  OK
;;; (define q (macroexpand '(let ((a a) (b a)) a))) q (ast-alpha-convert q)
;;; (define q (macroexpand '(f x (+ 2 3)))) q (ast-optimize q ())

;;; OK
;;; (ast-optimize (macroexpand '(cons 3 (begin2 (define-non-optimized x y) x 7))) ())
;;; [primitive #<2-ary primitive cons> [literal 3] [sequence [define x [variable y]] [variable x]]]

;;; OK
;;; (ast-optimize (macroexpand '(cons 3 (begin2 (define x y) x 7))) '(x))
;;; [sequence [define x [variable y]] [primitive #<2-ary primitive cons> [literal 3] [variable x]]]

;;; An important test:
;;; (ast-optimize (closure-body fibo) '(n))

;;; Something which should get smaller:
;;; (ast-optimize (closure-body ast-simplify-calls) '(ast))

;;; These are interesting because of the sequence in the let bound form:
;;; ACCEPTABLE(ast-optimize (macroexpand '(cons 3 (begin x 7))) ())
;;; GOOD(ast-optimize (macroexpand '(let ((a (newline) (newline) (newline))) y)) '())


;;; Primitive composition:
;; jitterlisp> (ast-optimize (macroexpand '(list 1 2)) '())
;; OK[let #<uninterned:0xe6e120> [primitive #<2-ary primitive cons> [literal 2] [literal ()]] [primitive #<2-ary primitive cons> [literal 1] [variable #<uninterned:0xe6e120>]]]

;; Make sure that this remains correct:
;; OK(ast-optimize (macroexpand '(let ((c 1)) (set! c 4) c)) '())
;;   { [let #<u235> [literal 1] [sequence [set! #<u235> [literal 4]] [variable #<u235>]]]
;;     CORRECT (difficult to optimize further without a special case). }

;; Nice testcases, containing many lets.  Those two should be rewritten
;; to different shapes:
;; (ast-optimize (macroexpand '(set 1 2 3 4)) ())
;; (ast-optimize (macroexpand '(set 1 x 3 4)) ())

;; Check that these are correctly optimized:
;; OK(ast-optimize (macroexpand '(begin2 x y)) ())
;;   {  [sequence [variable x] [variable y]] is CORRECT }
;; OK(ast-optimize (macroexpand '(begin1 x y z)) ())
;;   {  [sequence [variable y] [sequence [variable z] [variable x]]] is WRONG }
;; OK(ast-optimize (macroexpand '(begin2 x y z)) ())
;;   {  [sequence [variable x] [sequence [variable z] [variable y]]] is WRONG }
;; OK(ast-optimize (macroexpand '(let ((a (f 1)) (b a)) 4)) ())
;;   {  [let #<u263> [call [variable f] [literal 1]] [literal 4]] would be
;;      wrong: a is non-bound and non-constant, so its reference is effectful
;;      and cannot be optimized away.
;;      [sequence [call [variable f] [literal 1]] [sequence [variable a] [literal 4]]]
;;      is GOOD. }
;; GOOD(ast-optimize (macroexpand '(let* ((a (f 1)) (b a)) 4)) ())
;;   {  [let #<u267> [call [variable f] [literal 1]] [literal 4]] would be
;;      correct but subptimal: the let AST should become a sequence AST.
;;      [sequence [call [variable f] [literal 1]] [literal 4]] is GOOD.  }
;; OK(ast-optimize (macroexpand '(let* ((a (f 1)) (b a)) b)) ())
;;   {  [call [variable f] [literal 1]] }

;; It's important to rename nonglobals in the caller when optimizing closures:
;; otherwise, when inlining callees within the closure body some references to
;; globals might be captured by the closure formals.
;; FIXME: do I need to do a preliminary global alpha-convertion pass over all
;; closures before inlining for the first time?  I'm almost sure I don't, as
;; I always alpha-convert both the expression I am inlining *into* and the
;; callee body I'm copying before inlining.

;; FIXME: Make sure that when I remove a let binding a variable to an effectful
;; expression I check that the variables occurring free in the expression are
;; not assigned in the body *before* the variable use in the body.
;; [I don't rewrite such lets now, except in the easy case of wrappers; the
;;  current solution is therefore correct, even if not as good as it could be].

;; (ALL OK)Primitive optimization:
;;   (ast-optimize (macroexpand '(if (not a) b c)) ())
;;   (ast-optimize (macroexpand '(+ a 1)) ())
;;   (ast-optimize (macroexpand '(- a 1)) ())
;;   (ast-optimize (macroexpand '(= a 0)) ())
;;   (ast-optimize (macroexpand '(= 0 a)) ())



;; OK(ast-optimize (macroexpand '(if (not a) b c)) ())
;;     [if [variable a] [variable c] [variable b]]
;; OK(ast-optimize (macroexpand '(if (begin 1 2 3 a) b c)) ())
;;     [if [variable a] [variable b] [variable c]]
;; OK(ast-optimize (macroexpand '(if (begin 1 (f 2) 3 a) b c)) ())
;;     [sequence [call [variable f] [literal 2]] [if [variable a] [variable b] [variable c]]]


;; This must have no redundant lets...
;; (ast-optimize (macroexpand '(if (< n 2) a b) ) ())

;; (macroexpand '(letrec ((a a)) a))
;;     [let a [literal #<undefined>] [sequence [set! a [variable a]] [variable a]]]
;; OK(ast-optimize (macroexpand '(letrec ((a a)) a)) ())
;;     [literal #<undefined>]
;; Optimizing this may be just academic, but why not: a set! of a non-globally
;; bound variable to itself can be eliminated.

;; Why does this generate a let?
;; OK-UP-TO-HERE (macroexpand '(cadr q))
;;   [call [variable cadr] [variable q]]
;; SUBOPTIMAL! (ast-optimize (macroexpand '(cadr q)) ())
;;   [let #<u1812> [variable q] [primitive #<1-ary primitive car> [primitive #<1-ary primitive cdr> [variable #<u1812>]]]]
;; The problem is in closure-wrapper? :
;; jitterlisp> car
;; #<closure () (#<u701>) [primitive #<1-ary primitive car> [variable #<u701>]]>
;; jitterlisp> cadr
;; #<closure () (#<u1803>) [primitive #<1-ary primitive car> [primitive #<1-ary primitive cdr> [variable #<u1803>]]]>
;; jitterlisp> (closure-wrapper? car)
;; #t
;; jitterlisp> (closure-wrapper? cadr)
;; #f
;; Indeed, cadr is not a wrapper according to my definition, and my inlining
;; procedure for wrappers wouldn't work on it.

;; OK(ast-optimize (macroexpand '((lambda (x y) (+ x y)) 3 5)) ())
;;   {  [literal 8]  }
;; OK(ast-optimize (macroexpand '((lambda (x y) (+ x Q)) 3 5)) ())
;;   {  [primitive #<2-ary primitive primordial-+> [literal 3] [variable Q]]  }
;; OK(ast-optimize (macroexpand '((lambda (x y) (+ x 3)) 10 a)) ())
;;   {  [sequence [variable a] [literal 13]]  }

;; GOOD-EVEN-IF-IT-LOOKS-WRONG(to (let ((c '(1 2))) (set-car! c 42) c))
;;   {  This rewrites to
;;         [sequence [primitive #<2-ary primitive set-car!> [literal (1 2)] [literal 42]] [literal ...]]
;;      Notice the shared literal, which is of course the same list initially
;;      built as (1 2).  The block returns a literal, but the literal is
;;      actually a pointer to something which gets destructively changed, so the
;;      effect of the assignment is not lost.  As long as returning literals is
;;      efficient independently from their type (which has always been assumed
;;      up to this point -- even if it will almost certainly change with a
;;      moving GC) this is a good, fast solution.  JitterLisp conses are
;;      always mutable.
;;   }

;; FIXME: implement a few more rewritings.
;; optimizing this requires an equality predicate for ASTs.  In practice
;; it will be fast.
;; SUBOPTIMAL(ast-optimize (macroexpand '(lambda (a) (and a #f))) ())
;;   { [lambda (#<u1228>) [if [variable #<u1228>] [literal #f] [literal #f]]]
;;     This is always correct: [if E1 E2 E2] => [sequence E1 E2] }
;; A more subtle, but possibly more useful case to optimize:
;; SUBOPTIMAL(ast-optimize (macroexpand '(lambda (a) (and a a))) ())
;;   { [lambda (#<u1229>) [if [variable #<u1229>] [variable #<u1229>] [literal #f]]]
;;     Here the idea is that [if E E #f] can be rewritten to E when E is
;;     non-effectful.  }
;; SUBOPTIMAL(ast-optimize (macroexpand '(lambda (a) (lispy-or a a))) ())
;;   { [lambda (#<u1234>) [if [variable #<u1234>] [variable #<u1234>] [variable #<u1234>]]]
;;     There should be no need for an explicit rewrite
;;       [if E E E] => E when E is non-effectful
;;     ; this is subsumed by the new rule above and sequence semplification,
;;     using [sequence E E] as an intermediate step. }
;; After implementing the rule above check:
;; SUBOPTIMAL(ast-optimize (macroexpand '(lambda (a) (lispy-or a a a))) ())
;;   { [lambda (#<u1250>) [if [variable #<u1250>] [variable #<u1250>] [if [variable #<u1250>] [variable #<u1250>] [variable #<u1250>]]]] }


;; (and a b c) => (if a (if b c #f) #f)
;; (and a b c) => (not (or (not a) (not b) (not c)))
(define-macro (and2 . clauses)
  (if (null? clauses)
      '#t
      (let ((res-name (gensym)))
        `(let ((,res-name #t))
           (cond ,@(map (lambda (clause)
                          `((not ,clause)
                            (set! ,res-name #f)))
                        (all-but-last clauses))
                 (else
                  (set! ,res-name ,(last clauses))))
           ,res-name))))

;; (define-constant (length-do xs)
;;   (do ((res 0 (1+ res))
;;        (xs xs (cdr xs)))
;;       ((null? xs) res)))
;; (declaim ;;(ftype (function (list) fixnum) length-do)
;;          (optimize (safety 0) (debug 0) (space 0) (speed 3))
;;          (inline + -))
;; (defun length-do (xs)
;;   (declare (type list xs))
;;   (do ((res 0 (1+ res))
;;        (xs xs (cdr xs)))
;;       ((null xs) res)
;;     (declare (type fixnum res))
;;     ))
;; (defmacro while (guard &rest body-forms)
;;   `(do ()
;;        ((not ,guard))
;;      ,@body-forms))
;; (defun length-while (xs)
;;   (declare (type list xs))
;;   (let ((res 0))
;;     (declare (type fixnum res))
;;     (while (not (null xs))
;;       (setq res (1+ res))
;;       (setq xs (cdr xs)))
;;     res))

;; A good test case for the used-result flag.
;; (c (lambda (a b) (when a (set! b a)) b))

;; (when #f
;;   (define (f x y)
;;     (cons x y))
;;   f
;;   (closure-compile! f)
;;   (display (f 10 20))
;;   (newline)

;;   (define (g x y z)
;;     (display y))
;;   g
;;   (closure-compile! g)
;;   (display (g 10 20 30))
;;   (newline)
;;   )

;; Wrong result on PowerPC:
;; Q='bin/jitterlisp--unsafe--no-threading'; make -j && make -j $Q && time -p rj ./scripts/emulator $Q --colorize --no-omit-nothing --vm --repl --cross-disassemble --compact-uninterned --no-repl --eval '(disassemble gauss) (gauss 1)' 2>&1 
;;
;; The saved address on the return stack is wrong: the second one is equal to
;; the first.

;; Here the opcode is in r31 for debugging.  r20 is the link register.
;; r27 is the return stack top pointer.
;; This is good:
;; # 0x100b9bd4: procedure-prolog (8 bytes):
;;     0xf5d80000 92 9b 00 00      stw     r20,0(r27)
;;     0xf5d80004 3b e0 00 67      li      r31,103
;; ;; This is bad:
;; # 0x100b9bec: call/n1/retR 0xf5d80690 (136 bytes):
;;     ...
;;     # Beginning of the compiled-closure part.
;;     0xf5d80624 3b 7b 00 04      addi    r27,r27,4
;;     0xf5d80628 81 48 00 14      lwz     r10,20(r8)
;;     0xf5d8062c 7d 49 03 a6      mtctr   r10
;;     0xf5d80630 4e 80 04 20      bctr
;;     0xf5d80634 3b e0 00 0b      li      r31,11
;;     0xf5d80638 7e 14 83 78      mr      r20,r16
;;     # End of the compiled-closure part.
;;     ...
;; The link register is set *after* the call which is supposed to save
;; it to the return stack.

;; (disassemble-vm (let* ((a 1)) (lambda (x) (+ x a))))
;; (disassemble-vm (lambda (x) y))

;; Similar on x86_64, GCC 7 only:
;; # 0x55578e8: call-from-c/retR 0xffffffffffffffff (93 bytes):
;;     0x000000000402a000 48 8b 74 24 18       	movq   0x18(%rsp),%rsi
;;     0x000000000402a005 48 8b 54 24 10       	movq   0x10(%rsp),%rdx
;;     0x000000000402a00a 48 8b 06             	movq   (%rsi),%rax
;;     0x000000000402a00d 48 83 ee 08          	subq   $0x8,%rsi
;;     0x000000000402a011 48 89 74 24 18       	movq   %rsi,0x18(%rsp)
;;     0x000000000402a016 48 89 c1             	movq   %rax,%rcx
;;     0x000000000402a019 48 85 d2             	testq  %rdx,%rdx
;;     0x000000000402a01c 74 09                	je     0x000000000402a027
;;     0x000000000402a01e 48 6b d2 f8          	imulq  $0xfffffffffffffff8,%rdx,%rdx
;;     0x000000000402a022 48 8b 4c 16 08       	movq   0x8(%rsi,%rdx,1),%rcx
;;     0x000000000402a027 0f 1f 04 25 aa 00 00 00 	nopl   0xaa
;;     0x000000000402a02f 48 8b 74 24 20       	movq   0x20(%rsp),%rsi
;;     0x000000000402a034 48 8d 56 08          	leaq   0x8(%rsi),%rdx
;;     0x000000000402a038 0f 1f 04 25 bb 00 00 00 	nopl   0xbb
;;     0x000000000402a040 48 c7 46 08 aa aa 42 42 	movq   $0x4242aaaa,0x8(%rsi)
;;     0x000000000402a048 0f 1f 04 25 cc 00 00 00 	nopl   0xcc
;;     0x000000000402a050 ff 51 25             	callq  *0x25(%rcx)
;;     0x000000000402a053 48 89 54 24 20       	movq   %rdx,0x20(%rsp)
;;     0x000000000402a058 48 89 44 24 10       	movq   %rax,0x10(%rsp)
;; The two nopl instructions are for debugging.  The two movq are run too late,
;; after the call: 0x20(%rsp) is the return stack pointer, in this case held
;; in memory.  The instruction run after the call would have stored the correct
;; value if run before.
;;
;; Should JITTER_BRANCH_AND_LINK and JITTER_BRANCH_FAST_AND_LINK end with
;; __builtin_unreachable to prevent this kind of behavior?  I guess not:
;; sometimes I really want a jump instruction to be generated after the
;; branch-and-link instruction, to skip the rest of the VM instruction code
;; which is not empty.
;; Should JITTER_BRANCH_AND_LINK and JITTER_BRANCH_FAST_AND_LINK clobber
;; memory?  Would that help? [Tested: no it wouldn't]
;;
;; I have understood the problem now.
;; -fno-sched-interblock might work as a last-ditch workaround, but I should
;; do something more solid. [No, it wouldn't]

;; The GCC parameter max-goto-duplication-insns was defined following Anton
;; Ertl's complaint about unconditional branches to branches, which bothers
;; me as well, in https://gcc.gnu.org/bugzilla/show_bug.cgi?id=15242 .
;; It is supposed to be a solution.
;; https://gcc.gnu.org/bugzilla/show_bug.cgi?id=15242 reported by Bernd Paysan
;; is about similar issues, again also concerning me.

;; This fails: why?
;; (c ast-equal?)

;; MIPS: the 18 test suite failures are prevented by undefining
;; JITTER_MACHINE_SUPPORTS_PROCEDURE.  I have to understand why.

(when #f
  (define-constant (ap f x) (f x))
  (define (w) (ap 1+ 10))
  (disassemble-vm w)
  (disassemble-vm ap)
  )

;; As of 2018-03-02, after changing runtime definitions and code generation but before
;; cleaning them up, I'm seeing failures only on minimal-threading.  I think the problem
;; is in the call VM instruction: I don't see the link register in 0x10(%rsp) ever being
;; set.
(define-constant (CONS car cdr)
  (lambda (selector) (selector car cdr)))
(define-constant (CAR-SELECTOR car cdr)
  car)
(define-constant (CDR-SELECTOR car cdr)
  cdr)
(define-constant (CAR cons)
  (cons CAR-SELECTOR))
(define-constant (CDR cons)
  (cons CDR-SELECTOR))
(define-macro (LIST . args)
  (if (null? args)
      '()
      `(CONS ,(car args) (LIST ,@(cdr args)))))
