;;; This is a -*- Scheme -*- source file.
;;; A simple Scheme loop decrementing a variable from 2000000000 to 0.

;;; Copyright (C) 2016 Luca Saiu
;;; Written by Luca Saiu

;;; This file is part of GNU Jitter.

;;; Jitter is free software: you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation, either version 3 of the License, or
;;; (at your option) any later version.

;;; Jitter is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.

;;; You should have received a copy of the GNU General Public License
;;; along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>.


(define (count-down max)
  (let loop ((n max))
    (unless (zero? n)
      (loop (1- n)))))

;; ,disassemble count-down

(count-down 2000000000)
