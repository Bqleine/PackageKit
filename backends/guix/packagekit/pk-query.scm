;;; pk-query.scm --- Searching of packages with regexes and filters.

;; Copyright © 2024 Noé Lopez <noelopez@free.fr>
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

(define-module (packagekit pk-query)
  #:use-module ((gnu packages) #:select (fold-packages))
  #:use-module ((guix ui) #:select (package-relevance))
  #:use-module ((guix utils) #:select (version>?))
  #:use-module ((packagekit pk-profile) #:select (installed-packages))
  #:use-module (guix packages)
  #:use-module (ice-9 match)
  #:use-module (packagekit pk-id))

(define (package-installed? package installed)
  (let ((search (cons (package-name package)
		      (package-version package))))
    (not (not (member search installed)))))

;; TODO: use package cache.
(define-public (search-packages regexps)
  (define installed (installed-packages))
  (fold-packages
   (lambda (package result)
     (let ((relevance (package-relevance package regexps)))
       (cond
	((or (package-superseded package)
	     (zero? relevance))
	 result)
	(else
         (cons (cons (package->packagekit-id
		      package
		      #:installed?
		      (package-installed? package installed))
		     relevance)
               result)))))
   '()))

(define-public (sort-packages packages)
  (sort packages
          (lambda (m1 m2)
            (match m1
              ((package1 . score1)
               (match m2
                 ((package2 . score2)
                  (if (= score1 score2)
                      (if (string=? (package-name package1)
                                    (package-name package2))
                          (version>? (package-version package1)
                                     (package-version package2))
                          (string>? (package-name package1)
                                    (package-name package2)))
                      (> score1 score2)))))))))
