;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; $Id$
;;; Copyright (C) 1994, 1995 Free Software Foundation, Inc.
;;; Written by Sergey Poznyakoff.
;;; 
;;; This file is part of Cflow.
;;;  
;;; Cflow is free software; you can redistribute it and/or modify it
;;; under the terms of the GNU General Public License as published by the Free
;;; Software Foundation; either version 1, or (at your option) any later 
;;; version.
;;;
;;; Cflow is distributed in the hope that it will be useful, but
;;; WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
;;; or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
;;; for more details.
;;;
;;; You should have received a copy of the GNU General Public License along
;;; with GNU Smalltalk; see the file COPYING.  If not, write to the Free
;;; Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(require 'hilit19)
(provide 'hilit-cflow)

(hilit-set-mode-patterns
 'cflow-mode
 '(("\\(Direct Tree\\)\\|\\(Reverse Tree\\)" nil label)
   ("<[^>]*>" nil comment)
   ("[_a-zA-Z][_a-zA-Z0-9]*().*(recursive: see [0-9]+)" nil define)
   ("+-" nil keyword)
   ("|" nil keyword)
   (".*(R)$" nil include)))

