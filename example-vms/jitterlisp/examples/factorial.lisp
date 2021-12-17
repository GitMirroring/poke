;;; JitterLisp (almost -*- Scheme -*- for Emacs) down-counter example.

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


;; FIXME: comment for myself:
;;        2018-12-09: by replacing define with define-constant , which would be
;;        sensible, the recursive call compiles to call-compiled/n1/retR instead
;;        of call/n1/retR .  Currently call/n1/retR is much more sensitive than
;;        call-compiled/n1/retR : for example on PowerPC only the first is defective.
;;
;;        Unrelated:
;;        For performance's sake, I should recommend in the documentation that
;;        type and safety checks be separated from computation into separate VM
;;        instructions, so that rewrites can optimize the checks away.
(define-constant (fact n)
  (if (zero? n)
      1
      (* n (fact (1- n)))))

(display (fact 10))
(newline)
