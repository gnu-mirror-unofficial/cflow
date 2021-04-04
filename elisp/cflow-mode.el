;;; cflow.el --- major mode for viewing cflow output files.

;; Authors: 1994, 1995, 2005, 2007, 2021 Sergey Poznyakoff
;; Version: 0.2.1
;; Keywords: cflow

;; This file is part of GNU cflow
;; Copyright (C) 1994-2021 Sergey Poznyakoff
 
;; GNU cflow is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3 of the License, or
;; (at your option) any later version.
 
;; GNU cflow is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;; Installation:
;; You may wish to use precompiled version of the module. To create it
;; run:
;;    emacs -batch -f batch-byte-compile cflow-mode.el
;; Install the file cflow-mode.elc (and, optionally, cflow-mode.el) to
;; any directory in your Emacs load-path.

;; Customization:
;;  To your .emacs or site-start.el add:
;;  (autoload 'cflow-mode "cflow-mode")
;;  (setq auto-mode-alist (append auto-mode-alist
;;                                '(("\\.cflow$" . cflow-mode))))

(eval-when-compile
  ;; We use functions from these modules
  (mapcar 'require '(font-lock)))

(defvar cflow-mode-syntax-table nil
  "Syntax table used in cflow-mode buffers.")

(unless cflow-mode-syntax-table
  (setq cflow-mode-syntax-table (make-syntax-table))
  (modify-syntax-entry ?\# "<" cflow-mode-syntax-table)
  (modify-syntax-entry ?\n ">" cflow-mode-syntax-table))

  
(defvar cflow-mode-map (make-sparse-keymap)
  "Keymap used in Cflow mode.")

(define-key cflow-mode-map "s" 'cflow-find-function)
(define-key cflow-mode-map "r" 'cflow-recursion-root)
(define-key cflow-mode-map "R" 'cflow-recursion-next)
(define-key cflow-mode-map "x" 'cflow-goto-expand)
(define-key cflow-mode-map "E" 'cflow-edit-out-full)
(define-key cflow-mode-map "c" 'cflow-find-caller)
(define-key cflow-mode-map "n" 'cflow-next-function)
(define-key cflow-mode-map "p" 'cflow-prev-function)

(define-key cflow-mode-map [menu-bar] (make-sparse-keymap))

(define-key cflow-mode-map [menu-bar cflow]
  (cons "Cflow" (make-sparse-keymap "Cflow")))

(define-key cflow-mode-map [menu-bar cflow cflow-recursion-next]
  '("Recursion next" . cflow-recursion-next))
(define-key cflow-mode-map [menu-bar cflow cflow-recursion-root]
  '("Recursion root" . cflow-recursion-root))
(define-key cflow-mode-map [menu-bar cflow cflow-goto-expand]
  '("Find expansion" . cflow-goto-expand))
(define-key cflow-mode-map [menu-bar cflow cflow-find-function]
  '("Find function" . cflow-find-function))

;; Find the function under cursor.
;; Switch to the proper buffer and go to the function header
(defun cflow-find-function ()
  (interactive)
  (let ((lst (cflow-find-default-function)))
    (cond
     (lst
      (switch-to-buffer (find-file-noselect (car lst)))
      (goto-line (car (cdr lst))))
     (t
      (error "No source/line information for this function.")))))

;; Parse a cflow listing line
;; Return (list SOURCE-NAME LINE-NUMBER)
(defun cflow-find-default-function ()
  (save-excursion
    (beginning-of-line)
    (cond
     ((re-search-forward "\\([^ \t:]+\\):\\([0-9]+\\)"
			 (save-excursion (end-of-line) (point))
			 t)
      (list
       (buffer-substring (match-beginning 1) (match-end 1))
       (string-to-number
	(buffer-substring (match-beginning 2) (match-end 2)))))
     (t
      nil))))

;; If the cursor stays on a recursive call, then go to the root of
;; this call
(defun cflow-recursion-root ()
  (interactive)
  (let ((num (cond
	      ((save-excursion
		 (beginning-of-line)
		 (re-search-forward "(recursive: see \\([0-9]+\\))"
				    (save-excursion (end-of-line) (point))
				    t))
	       (string-to-number
		(buffer-substring (match-beginning 1) (match-end 1))))
	      (t
	       0))))
    (cond
     ((> num 0)
      (push-mark)
      (goto-line num))
     (t
      (error "Not a recursive call")))))

(defun cflow-recursion-next ()
  "Go to next recursive call"
  (interactive)
  (save-excursion
    (beginning-of-line)
    (cond
     ((re-search-forward "(R)"
			 (save-excursion (end-of-line) (point)) t)
      (setq cflow-recursion-root-line (count-lines 1 (point))))))
  (cond
   ((null cflow-recursion-root-line)
    (error "No recursive functions"))
   (t
    (let ((pos (save-excursion
		 (next-line 1)
		 (re-search-forward
		  (concat "(recursive: see "
			  (number-to-string cflow-recursion-root-line)
			  ")")
		  (point-max)
		  t))))
      (cond
       ((null pos)
	(goto-line cflow-recursion-root-line)
	(error "No more calls."))
       (t
	(push-mark)
	(goto-char pos)
	(beginning-of-line)))))))

(defun cflow-goto-expand ()
  (interactive)
  (let ((num (cond
	      ((save-excursion
		 (beginning-of-line)
		 (re-search-forward "\\[see \\([0-9]+\\)\\]"
				    (save-excursion (end-of-line) (point))
				    t))
	       (string-to-number
		(buffer-substring (match-beginning 1) (match-end 1))))
	      (t
	       0))))
    (cond
     ((> num 0)
      (push-mark)
      (goto-line num))
     (t
      (error "Not expandable")))))

(defun cflow-find-caller ()
  "Go to the caller of the current function"
  (interactive)
  (let ((num (save-excursion
	       (beginning-of-line)
	       (cond
		((looking-at "^\\( +\\)\\S-")
		 (let ((indent (- (match-end 1) (match-beginning 1))))
		   (re-search-backward (format "^ \\{,%d\\}\\S-" (- indent 1)) nil t)
		   (- (match-end 0) 1)))
		((looking-at "^\\(\\s-+\\)\\S-")
		 (let ((indent (- (match-end 1) (match-beginning 1))))
		   (catch 'found
		     (while (re-search-backward "^\\(\\s-*\\)\\S-" nil t)
		       (if (< (- (match-end 1) (match-beginning 1)) indent)
			   (throw 'found (match-end 1))))
		     0)))
		(t
		 0)))))
    (cond
     ((> num 0)
      (push-mark)
      (goto-char num))
     (t
      (error "At top-level function")))))

(defun cflow-goto-sibling (count advance)
  (let ((pos (save-excursion
	       (beginning-of-line)
	       (cond
		((looking-at "^\\( +\\)\\S-")
		 (let ((indent (- (match-end 1) (match-beginning 1)))
		       (rx (format "^%s\\S-" (match-string 0))))
		   (let ((pos nil)
			 (stop (if (> advance 0) 'eobp 'bobp)))
		     (while (not pos)
		       (if (funcall stop)
			   (error "No more calls"))
		       (forward-line advance)
		       (cond
			((looking-at "^\\( *\\)\\S-")
			 (let ((l (- (match-end 1) (match-beginning 1))))
			   (cond
			    ((= l indent)
			     (setq count (1- count))
			     (if (= count 0)
				 (setq pos (match-end 1))))
			    ((< l indent)
			     (error "No more calls")))))))
		     pos)))
		((looking-at "^\\S-")
		 (error "At top-level function"))
		(t
		 (error "Not at function"))))))
    (goto-char pos)))

(defun cflow-next-function (&optional arg)
  "Go to the next function at this nesting level"
  (interactive "P")
  (cflow-goto-sibling (or arg 1) 1))

(defun cflow-prev-function (&optional arg)
  "Go to the previous function at this nesting level"
  (interactive "P")
  (cflow-goto-sibling (or arg 1) -1))

(defvar cflow-read-only)

(defun cflow-edit-out-full ()
  "Get out of Cflow mode, leaving Cflow file buffer in fundamental mode."
  (interactive)
  (if (yes-or-no-p "Should I let you edit the whole Cflow file? ")
      (progn
	(setq buffer-read-only cflow-read-only)
	(fundamental-mode)
	(message "Type 'M-x cflow-mode RET' once done"))))


;; Font-lock stuff
(defconst cflow-font-lock-keywords
  (eval-when-compile
    (list
     (cons "^\\s *[0-9]+" font-lock-constant-face)
     (list "\\(\\S +\\)()\\s +\\(<[^>]*>\\)"
	   '(1 font-lock-function-name-face)
	   '(2 font-lock-type-face))
     (list "\\(\\S +\\)\\s +\\(<[^>]*>\\)"
	   '(1 font-lock-variable-name-face)
	   '(2 font-lock-type-face))
     (cons "\\S +()$" font-lock-builtin-face)
     (cons "(R):?$" font-lock-comment-face)
     (cons "(recursive: see [0-9]+)" font-lock-comment-face)
     (cons "^[ \\t+-|\\]+" font-lock-keyword-face))))

;;;###autoload
(defun cflow-mode ()
  "Major mode for viewing cflow output files

Key bindings are:
\\{cflow-mode-map}
"
  (interactive)
  (kill-all-local-variables)
  (use-local-map cflow-mode-map)
  (setq major-mode 'cflow-mode
	mode-name "Cflow")
  (make-variable-buffer-local 'cflow-recursion-root-line)

  (set (make-local-variable 'cflow-read-only) buffer-read-only)
  (setq buffer-read-only t)

  (set-default 'cflow-recursion-root-line nil)

  (set-syntax-table cflow-mode-syntax-table)

  (make-local-variable 'font-lock-defaults)
  (setq font-lock-defaults
	'((cflow-font-lock-keywords) nil t
	  (("+-*/.<>=!?$%_&~^:" . "w"))
	  beginning-of-line)))

(provide 'cflow-mode)
;;; cflow-mode ends




