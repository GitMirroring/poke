;;; jitter-mode.el --- Major mode for editing Jitter VM specifications.

;; Copyright (C) 2017, 2018, 2019, 2021 Luca Saiu
;; Updated in 2020 by Luca Saiu
;; Written by Luca Saiu

;; Maintainer: Luca Saiu

;; This file is part of Jitter.  It is NOT part of GNU Emacs.

;; Jitter is free software: you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.

;; Jitter is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with Jitter.  If not, see <http://www.gnu.org/licenses/>. */


;;;; Utility and debug.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;; People commonly with-demoted-errors in Major modes, but I hesistate,
;;; particularly now that the code is new and I need to see any error.  I should
;;; probably switch to really use this, later.  Right how I do the exact
;;; opposite: I evaluate the forms in a dynamic extent where debug-on-error is t
;;; , to make really sure to see things break when they do.
;;;
;;; I can use the global jitter-with-demoted-errors to change this behavior.
(defvar jitter-with-demoted-errors nil)
(defmacro jitter-with-demoted-errors (message &rest forms)
  "When jitter-with-demoted-errors is non-nil behave like
with-demoted-errors in a context where debug-on-error is nil; if
jitter-with-demoted-errors is nil evaluate the forms in a context
where debug-on-error is t ."
  `(if jitter-with-demoted-errors
       (let ((debug-on-error nil))
         (with-demoted-errors ,message
           ,@forms))
       (let ((debug-on-error t))
         ,@forms)))

(defun jitter-debug (&rest stuff)
;;  (apply 'message stuff)
  ;;(sit-for 0)
  )

;;; Avoid byte-compiler warnings about free variables; these are dynamically
;;; bound and not defined as globals, but my way of using them is perfectly
;;; safe.
(defvar font-lock-beg)
(defvar font-lock-end)

(defun jitter-show-block ()
  "A convenient visual way to test
`jitter-font-lock-extend-region-to-toplevel-block'."
  (interactive)
  (save-excursion
    (let* ((point (point))
           (font-lock-beg point)
           (font-lock-end font-lock-beg)
           (changed (jitter-font-lock-extend-region-to-toplevel-block)))
      (dotimes (i 3)
        (goto-char font-lock-beg)
        (sit-for 0.5)
        (goto-char font-lock-end)
        (sit-for 0.5))
;;      (message "%S -> (%S, %S) %s"
;;               point font-lock-beg font-lock-end
;;               (if changed "CHANGED" "NOT changed"))
      )))




;;;; Font locking: font-lock-extend-region for Jitter
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun jitter-font-lock-extend-region-brutal ()
  "Extend the font-lock region to cover one or two Jitter toplevel blocks.
This works by redefining the dynamically-bound variables
`font-lock-beg' and `font-lock-end', and returning non-nil in
case of changes.  This version brutally extends the fontification
region to the whole buffer, which is always correct but almost
always suboptimal; anyway it comes in handy for testing in a
favorable case."
  (let ((old-font-lock-beg font-lock-beg)
        (old-font-lock-end font-lock-end)
        (min (point-min))
        (max (point-max)))
        ;;; Return non-nil iff an adjustment was made.
    (if (and (= old-font-lock-beg min)
             (= old-font-lock-end max))
        nil
      (progn
        (setq font-lock-beg min
              font-lock-end max)
        t))))

;;; A buffer is conceptually divided into "blocks".  The first block starts at
;;; (point-min), and the others *right after* the end of the previous one.  Each
;;; block ends after the last character of an "^end" line or, in the case of the
;;; last one, at (point-max).  You can see block boundaries interactively with
;;; the command `jitter-show-block'.

;;; Blocks are asymmetrical: they start at column zero, but end at the last
;;; column of a line.  This is useful to maintain the property that every
;;; position in a buffer belongs to one and only one block.

;;; Jitter's syntax has the nice property of allowing blocks to be font-locked
;;; one at the time, independently from one another: we exploit it in the
;;; `font-lock-extend-region' function, to keep the part to refontify as small
;;; as possible.

(defconst jitter-block-end-regexp
  "^end[ \t]*\\(?:\\(?:#.*\\)?\\)$")

(defun jitter-block-beginning (position)
  "Return the beginning position of the block containing POSITION,
according to the criteria descrived in the comment above.  This
function moves the point, since it is called in a context where
that has no effect."
  ;; The block containing position ends at the end of the terminator line after
  ;; position: if position is right at the end of a terminator line, the
  ;; backward search should not find its line, but the *previous* terminator, as
  ;; per the definition of block above; this is why we move to the beginning of
  ;; the current line before searching backward.
  (goto-char position)
  (beginning-of-line)
  (or (when (re-search-backward jitter-block-end-regexp nil t)
        ;; We have found the end of an "^end" line.  Move to the next character,
        ;; which will be the first of a new line.  That is the block beginning.
        (forward-line)
        (point))
      (point-min)))

;;; FIXME: I might want to redefine this more simply, in terms of
;;; jitter-block-beginning for the next block, backing by one character
;;; unless the end of the buffer is reached.
(defun jitter-block-end (position)
  "Return the end position of the block containing POSITION,
according to the criteria descrived in the comment above.  This
function moves the point, since it is called in a context where
that has no effect."
  (goto-char position)
  ;; The block ends at the end of the "^end" which *ends* before position: in
  ;; particular, if position is within the terminator line, it is the end of
  ;; *that* line which marks the end of the block; we don't want the search not
  ;; to find the right terminator just because position is already past the
  ;; first character of the terminator line.  For this reason we move to the
  ;; beginning of the line before searching forward.
  (beginning-of-line)
  (or (when (re-search-forward jitter-block-end-regexp nil t)
        ;; We are already at the end of the line, beacause of the regexp.  We do
        ;; not want to go to the beginning of the next line, to keep the
        ;; asymmetry property discussed in the comment above.
        (point))
      (point-max)))

(defun jitter-font-lock-extend-region-to-toplevel-block ()
  "Extend the font-lock region to cover one or two Jitter toplevel blocks.
This works by redefining the dynamically-bound variables `font-lock-beg' and
`font-lock-end', and returning non-nil in case of changes.  This
covers cover at least a toplevel block; if a full block does not
exist in either direcion, extend to that end of the buffer."
    (let* ((old-point (point))
           (new-font-lock-beg (jitter-block-beginning font-lock-beg))
           (new-font-lock-end (jitter-block-end font-lock-end)))
      ;;(assert (<= new-font-lock-beg new-font-lock-end))
      (jitter-debug "Point at %S.  Moving font-lock-beg, font-lock-end from %S, %S to %S, %S"
                    old-point
                    font-lock-beg font-lock-end
                    new-font-lock-beg new-font-lock-end)
        ;;; Return non-nil iff an adjustment was made.
      (if (and (= font-lock-beg new-font-lock-beg)
               (= font-lock-end new-font-lock-end))
          nil
        (progn
          (setq font-lock-beg new-font-lock-beg
                font-lock-end new-font-lock-end)
          t))))

(defun jitter-font-lock-extend-region ()
  "Extend the font-lock region to cover one or two Jitter toplevel blocks.
This works by redefining the dynamically-bound variables
`font-lock-beg' and `font-lock-end', and returning non-nil in
case of changes."
  ;;(jitter-font-lock-extend-region-brutal)
  (jitter-font-lock-extend-region-to-toplevel-block)
  )




;;;; Jitter major mode definition.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-derived-mode jitter-mode prog-mode "Jitter"
  "Major mode for editing Jitter VM specification.

Right now the mode only provides font-locking support.

\\{jitter-mode-map}"
  (defconst jitter-font-lock-keywords
    `(;; Highlight C code.  Anything coming in the same line after "code",
      ;; before an optional comment, is highlighted as wrong (by contrast, a
      ;; non-comment right after an "end" following "code" is just part of the C
      ;; code--it's not recognized as the end of the block, but is certainly not
      ;; an error).
      ("^[^#\n]*\\<code\\>[ \t]*\\([^\n#]*\\)\\(\\(?:#.*\\)?\\)\n\\(\\(?:.\\|\n\\)*?\\)\n[ \t]*\\<end\\>[ \t]*\\(\\(?:#.*\\)?\\)$"
       (1 font-lock-warning-face prepend)
       (2 font-lock-comment-face prepend)
       (3 'default append)
       (4 font-lock-comment-face prepend))

      ;; A non-comment non-whitespace character right after a non-commented
      ;; "end" in the same line is wrong out of "code" blocks.  This will not
      ;; fire within "code" blocks, where something following "end" is
      ;; (correctly) recognized as part of the C code.  Still, this is useful
      ;; elsewhere.
      ("^[^#\n]*?\\<end\\>[ \t]*\\([^\n#]*\\)\\(\\(?:#.*\\)?\\)$"
       (1 font-lock-warning-face nil)
       (2 font-lock-comment-face nil))

      ;; Highlight comments.
      ("#.*$"
       0 font-lock-comment-face nil)

      ;; Highlight argument mode and kind.
      ("\\([?!]+[ \t]*\\<[nlfA-Z]+\\>\\)"
       1 font-lock-type-face nil)

      ;; Highlight numeral literals.
      ("\\([-+]?\\(?:\\(?:0b[01]+\\)\\|\\(?:0o?[0-7]+\\)\\|\\(?:0x[0-9a-fA-F]+\\)\\|\\(?:0\\|\\(?:[1-9][0-9]*\\)\\)\\)[uU]?\\)"
       1 font-lock-constant-face nil)

      ;; Highlight numeral-like constants.
      ("BITSPERWORD\\|BYTESPERWORD\\|LGBYTESPERWORD"
       0 font-lock-constant-face t)

      ;; Highlight string literals.
      ("\"\\(?:[^\"\\\\]\\|\\\\.\\)*\""
       0 font-lock-string-face nil)

      ;; Highlight Jitter argument macros in C code.
      ("\\<JITTER_ARG[NUPF]?\\(?:0\\|[1-9][0-9]*\\)\\>"
       0 ;;font-lock-variable-name-face
         font-lock-preprocessor-face
         t)

      ;; Highlight placeholders in rewrite rules.
      ("\\<\\$[^ \t\n]+\\>"
       0 ;;font-lock-variable-name-face
         font-lock-preprocessor-face
         t)

      ;; Highlight Jitter statement-like macros in C code.
      ("\\<JITTER_EXIT\\>"
       0 font-lock-builtin-face t)
      ("\\<JITTER_BRANCH\\(?:_FAST\\)?\\(?:_AND_LINK\\(?:_WITH\\)?\\)?\\>"
       0 font-lock-builtin-face t)
      ("\\<JITTER_CALL_C\\>\\|\\<JITTER_BRANCH_\\(FAST_\\)?IF_\\(?:ZERO\\|NONZERO\\|POSITIVE\\|NONPOSITIVE\\|NEGATIVE\\|NONNEGATIVE\\|EQUAL\\|NOTEQUAL\\|LESS_UNSIGNED\\|LESS_SIGNED\\|GREATER_UNSIGNED\\|GREATER_SIGNED\\|NOTLESS_UNSIGNED\\|NOTLESS_SIGNED\\|NOTGREATER_UNSIGNED\\|NOTGREATER_SIGNED\\|AND\\|NOTAND\\|\\(\\(PLUS\\|MINUS\\|TIMES\\|DIVIDED\\|REMAINDER\\|NEGATE\\)_OVERFLOWS\\)\\)\\>"
       0 font-lock-builtin-face t)
      ("\\<JITTER_\\(PLUS\\|MINUS\\|TIMES\\|DIVIDED\\|REMAINDER\\|NEGATE\\)_BRANCH_\\(FAST_\\)?IF_OVERFLOW\\>"
       0 font-lock-builtin-face t)

      ;; Highlight other machine-defined Jitter macros for instruction bodies.
      ("\\<JITTER_SPECIALIZED_INSTRUCTION_OPCODE\\>"
       0 font-lock-preprocessor-face
         t)

      ;; Highlight the text to be replaced with a VM-specific prefix in the
      ;; outout.  This must *not* be word-delimited, since the replacement
      ;; will fire in any context.
      ("vmprefix_\\|VMPREFIX_"
       0 font-lock-preprocessor-face t)

      ;; Highlight global VM parameter names coming after "set".
      ("^[^#\n]*\\<set\\>[ \t\n]+\\([a-zA-Z_][a-zA-Z_0-9]*\\)\\>"
       1 font-lock-variable-name-face t)

      ;; Highlight instruction names.
      ("^[^#\n]*\\<instruction\\>[ \t\n]+\\<\\([_a-zA-Z][-+._~@/\\\\a-zA-Z0-9]*\\)\\>[ \t\n]*("
       1 font-lock-function-name-face t)

      ;; Highlight register class letter.
      ("^[^#\n]*\\<register-class\\>[ \t\n]+\\<\\([a-z]\\)\\>"
       1 font-lock-function-name-face t)

      ;; Highlight stack letter.
      ("^[^#\n]*\\<stack\\>[ \t\n]+\\<\\([a-z]\\)\\>"
       1 font-lock-function-name-face t)

      ;; Highlight wrapped globals.  This is not 100% satisfactory, as it does
      ;; not font-lock comments within the block; however that cannot work with
      ;; the same form I'm using, since there could be an indeterminate number
      ;; of matches within each block.
      ("^[^#\n]*\\<wrapped-globals\\>[ \t\n]+\\(\\(?:[^ \t\n]+[ \t\n]+\\)*?\\)\\<end\\>"
       (1 font-lock-variable-name-face t))

      ;; Highlight wrapped functions.  See the comment above about the comment
      ;; restriction.
      ("^[^#\n]*\\<wrapped-functions\\>[ \t\n]+\\(\\(?:[^ \t\n]+[ \t\n]+\\)*?\\)\\<end\\>"
       1 font-lock-function-name-face t)

      ;; Highlight keywords.
      ("\\(set\\|register-class\\|fast-register-no\\|no-slow-registers\\|slow-registers\\|stack\\|letter\\|long-name\\|c-type\\|element-no\\|c-initial-value\\|c-element-type\\|non-tos-optimized\\|tos-optimized\\|guard-underflow\\|guard-overflow\\|no-guard-underflow\\|no-guard-overflow\\|initial-header-c\\|early-header-c\\|initial-vm-main-c\\|initial-vm1-c\\|initial-vm2-c\\|initial-vmmain-c\\|printer-c\\|rewriter-c\\|early-c\\|late-c\\|initialization-c\\|finalization-c\\|state-struct-backing-c\\|state-struct-runtime-c\\|state-initialization-c\\|state-reset-c\\|state-finalization-c\\|instruction-beginning-c\\|instruction-end-c\\|wrapped-functions\\|wrapped-globals\\|instruction\\|hot\\|cold\\|relocatable\\|non-relocatable\\|non-branching\\|branching\\|caller\\|callee\\|returning\\|code\\|end\\|vm\\)"
       (0 font-lock-keyword-face nil))

      ;; Highlight rule names.
      ;;("^[^#\n]*\\<rule\\>[ \t\n]+\\<\\([_a-zA-Z][-+_~@/\\\\a-zA-Z0-9]*\\)\\>" ;; \\)\\>[ \t\n]*("
      ("^[^#\n]*\\<rule\\>[ \t]+\\([_a-zA-Z][-+._~@/\\\\a-zA-Z0-9]*\\)[ \t]*$" ;; \\)\\>[ \t\n]*("
       1 font-lock-preprocessor-face t)

      ;; Experimental rule-only keywords, to merge with the rule before; some of these should
      ;; only be font-locked this way when they appear in the right context, if feasible.
      ;;("\\<\\(rewrite\\|rule\\|turn\\|into\\|and\\|or\\|not\\|equals\\|when\\|has-property\\|is-literal\\|is-register\\|is-label\\|is-fast-label\\|is\\|precedes\\)\\>"
      ;; (0 font-lock-keyword-face nil))
      ("\\<\\(rewrite\\|rule\\|into\\|when\\)\\>"
       (0 font-lock-keyword-face nil))

      ;; Experimental patterns, to merge with the rule before.
      ("\\(\\$\\(?:0\\|[1-9][0-9]*\\)\\)"
       1 font-lock-preprocessor-face t)

      ;; Highlight stack operation prefixes, for instruction bodies.
      ("\\<\\(JITTER_\\(?:TOP\\|UNDER_TOP\\|AT_NONZERO_DEPTH\\|AT_DEPTH\\|AT_NONZERO_DEPTH\\|SET_AT_DEPTH\\|SET_AT_NONZERO_DEPTH\\|PUSH_UNSPECIFIED\\|PUSH\\|UNDER_PUSH_UNSPECIFIED\\|UNDER_PUSH\\|DROP\\|DUP\\|SWAP\\|QUAKE\\|OVER\\|TUCK\\|NIP\\|ROT\\|MROT\\|ROLL\\|MROLL\\|SLIDE\\|WHIRL\\|BULGE\\|UNARY\\|BINARY\\|REVERSE\\|HEIGHT\\|SET_HEIGHT\\)_\\)\\([A-Z]+\\)\\>"
       (1 font-lock-preprocessor-face t)
       ;;(2 font-lock-variable-name-face t)
       )
      ))

  (setq font-lock-defaults
        '(jitter-font-lock-keywords
          t    ; keywords-only: if non-nil don't do syntactic fontifications on strings and comments
          nil  ; case-fold
          nil  ; syntax-alist
          ;; Fontification should be performed syntactically, not limited to the
          ;; current line.  This is needed because of the relatively complex
          ;; "code" syntax.
          (jit-lock-contextually . t)

          ;; These will probably not be needed.
          ;;(font-lock-extend-region-functions . (jitter-extend-region))
          ;;(font-lock-unfontify-region-function . jitter-unfontify-region)
          ))

  ;; By default the `font-lock-extend-region-functions' hook contains
  ;; `font-lock-extend-region-wholelines' and
  ;; `font-lock-extend-region-multiline'.  I absolutely don't want the first,
  ;; because it would defeat my strategy of breaking the buffer into blocks
  ;; terminated by "^end" lines, which *must* be asymmetrical -- see the comment
  ;; above.
  (setq font-lock-extend-region-functions
        '(jitter-font-lock-extend-region
          font-lock-extend-region-multiline))

  (jitter-with-demoted-errors "jitter: %S"
    ;; We rely on jit-lock-contextually to re-font-lock the a modified region
    ;; with its context.
    (setq-local jit-lock-contextually 'syntax-driven)

    ;; We definitely want to fontify a widened buffer.
    (setq-local font-lock-dont-widen nil)

    (jitter-debug "ENTERING JITTER MODE")))




;;;; Notes for myself.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; FIXME: What about two column-0 "end" lines, one directly after the other?
;;; That would be worth supporting.

;;; Support comment commands.  Test with M-j  ,  C-x ;  .

;;; comment-start-skip , comment-start , comment-end , comment-multi-line ,
;;; comment-indent-function seem the right solution.  But I'm too tired now.

;;; completion-at-point-functions : this looks easy.

;;; See (elisp) §22.2.1 Major Mode Conventions.

;;; jitter-mode-hook is automatically defined .  Does it actually run hooks by
;;; calling run-mode-hooks ?  I should add a hook and see if it works.

;;; Ideally, I should make sure this does the right thing: `syntax-ppss'.  It
;;; computed "the syntactic state corresponding to a given buffer position",
;;; which is useful for indentation.  Jitter's syntax should be friendly to it.
;;; The return value of `syntax-ppss' is supposed to be "the same as if you call
;;; the low-level parsing function ‘parse-partial-sexp’ from the beginning of
;;; the buffer to POS, but using and updating a cache for efficiency.  See
;;; (elisp)§34.6.2 and the following sections.

;;; Indentation: see the variables `electric-indent-chars'
;;; `electric-indent-inhibit' (let it remain nil) and the functions
;;; `indent-line-function'.  SMIE, based on operator precedence parsing,
;;; (elisp)§22.7.1"Simple Minded Indentation Engine" might work for me: it
;;; automatically provides ‘forward-sexp’ ‘backward-sexp’ and `transpose-sexps'
;;; and works well with syntactically-incorrect code.

;;; Without SMIE (since Jitter has a simple syntax), I shoud look at
;;; (elisp)§31.17.2"Indentation Controlled by Major Mode".  There is
;;; even support for indenting embedded code in a different language!




;;;; Provide.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(provide 'jitter-mode)




;;;; Footer.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; jitter-mode.el ends here
