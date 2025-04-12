;;; pk-filters.scm --- Matching packages with filters.

;; Copyright © 2025 Noé Lopez <noelopez@free.fr>
;;
;; Licensed under the GNU General Public License Version 2
;;
;; This program is free software; you can redistribute it and/or
;; modify it under the terms of the GNU General Public License as
;; published by the Free Software Foundation; either version 2 of the
;; License, or (at your option) any later version.
;;
;; This program is distributed in the hope that it will be useful, but
;; WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;; General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with this program. If not, see
;; <https://www.gnu.org/licenses/>.

(define-module (packagekit pk-filters)
  #:use-module ((packagekit pk-profile) #:select (installed-packages))
  #:use-module ((guix packages) #:select (supported-package?
					  package-name
					  package-version))
  #:use-module (srfi srfi-1))

(define-public (package-installed? package)
  "If PACKAGE is installed."
  (define installed (installed-packages)) ;TODO: cache this
  (let ((search (cons (package-name package) (package-version package))))
    (->bool (member search installed))))

(define %supported-filters
  `((installed . ,package-installed?)
    (arch . ,supported-package?)))

(define-public (parse-filters str)
  "Parses a packagekit filter string into a list of filters."
  (fold
   (lambda (str alist)
     (let* ((negated? (char=? (string-ref str 0) #\~))
	    (filter (if negated? (string-drop str 1) str))
	    (sym (string->symbol filter)))
       (if (assq sym %supported-filters)
	   (alist-cons sym (not negated?) alist)
	   alist)))
   '()
   (string-split str #\;)))

(define-public (filter-packages packages filters)
  "Applies FILTERS to the list of PACKAGES."
  (define (xor a b)
    (if a (if b #t #f) (if b #f #t)))

  (fold
   (lambda (filter packages)
     (let ((predicate (assq-ref %supported-filters (car filter)))
	   (negated? (cdr filter)))
       (unless predicate
	 (error "unsupported filter"))
       (fold
	(lambda (package filtered)
	  (if (xor (predicate package) negated?)
	      (cons package filtered)
	      filtered))
	'()
	packages)))
   packages
   filters))
