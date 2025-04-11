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
  #:use-module ((gnu packages) #:select (fold-packages fold-available-packages))
  #:use-module ((guix ui) #:select (package-relevance))
  #:use-module ((guix utils) #:select (version>?))
  #:use-module ((ice-9 regex) #:select (fold-matches match:substring))
  #:use-module ((packagekit pk-profile) #:select (installed-packages))
  #:use-module (guix packages)
  #:use-module (ice-9 match)
  #:use-module (packagekit pk-id))

(define (package-installed? name version installed)
  (let ((search (cons name version)))
    (->bool (member search installed))))

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
		      (package-installed? (package-name package)
					  (package-version package)
					  installed))
		     relevance)
               result)))))
   '()))

(define-public (search-packages-fast regexps)
  "Like search-packages but uses the package cache.  It is limited to
 only searching by name."
  (define installed (installed-packages))
  (define (name-score regexp name)
    (fold-matches regexp name 0
                  (lambda (m score)
                    (+ score
                       (if (string=? (match:substring m) name)
                           5             ;exact match
                           1)))))
  (define (name-relevance regexps name)
    (let loop ((regexps regexps)
               (total-score 0))
      (match regexps
	((head . tail)
	 (let ((score (name-score head name)))
           ;; Return zero if one of PATTERNS doesn't match.
           (if (zero? score)
               0
               (loop tail (+ total-score score)))))
	(() total-score))))

  (fold-available-packages
   (lambda* (name version result #:key deprecated? #:allow-other-keys)
     (let ((relevance (name-relevance regexps name)))
       (cond
	((or deprecated?
	     (zero? relevance))
	 result)
	(else
         (cons (cons (packagekit-id
		      name
		      version
		      #:installed?
		      (package-installed? name version installed))
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
                      (if (string=? (packagekit-id-name package1)
                                    (packagekit-id-name package2))
                          (version>? (packagekit-id-version package1)
                                     (packagekit-id-version package2))
                          (string>? (packagekit-id-name package1)
                                    (packagekit-id-name package2)))
                      (> score1 score2)))))))))
