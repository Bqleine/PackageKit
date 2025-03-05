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

(define-module (packagekit pk-guile-interface)
  #:use-module (gnu packages)
  #:use-module (gnu packages base)
  #:use-module (guix packages)
  #:use-module (guix scripts show)
  #:use-module (guix utils)
  #:use-module ((guix licenses) #:select (license-name license?))
  #:use-module (guix ui)
  #:use-module (ice-9 match)
  #:use-module (ice-9 and-let-star)
  #:use-module (srfi srfi-1)
  #:use-module (srfi srfi-11)
  #:use-module (packagekit pk-profile)
  #:use-module (packagekit pk-id))

;; (define (find-packages-by-description regexps)
;;   "Return a list of pairs: packages whose name, synopsis, description,
;; or output matches at least one of REGEXPS sorted by relevance, and its
;; non-zero relevance score."
;;   (let ((matches (fold-packages (lambda (package result)
;;                                   (if (package-superseded package)
;;                                       result
;;                                       (match (package-relevance package
;;                                                                 regexps)
;;                                         ((? zero?)
;;                                          result)
;;                                         (score
;;                                          (cons (cons package score)
;;                                                result)))))
;;                                 '())))
;;     (sort matches
;;           (lambda (m1 m2)
;;             (match m1
;;               ((package1 . score1)
;;                (match m2
;;                  ((package2 . score2)
;;                   (if (= score1 score2)
;;                       (if (string=? (package-name package1)
;;                                     (package-name package2))
;;                           (version>? (package-version package1)
;;                                      (package-version package2))
;;                           (string>? (package-name package1)
;;                                     (package-name package2)))
;;                       (> score1 score2))))))))))

(define (search-packages regexps)
  (fold-packages (lambda (package result)
                   (if (package-superseded package)
                       result
                       (match (package-relevance package
                                                 regexps)
                         ((? zero?)
                          result)
                         (score
                          (cons (cons package score)
                                result)))))
                 '()))

(define (sort-packages packages)
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

(define (package-id package)
  (string-append (package-name package)
			 ";"
			 (package-version package)
			 ";"
			 ";"))

(define (make-package-result packages)
  (map (lambda (package)
	 (cons
	  (package-id package)
	  (package-description package)))
       packages))

(define (package->license-string package)
  ;; TODO: use licenses->project-license from #76661
  (let ((licenses (package-license package)))
    (cond
     ((list? licenses)
      (license-name (car licenses)))
     ((license? licenses)
      (license-name licenses))
     (else
      "unknown license"))))

(define (make-package-details-result package)
  (list (package-id package)
	(package-description package)
	(package-synopsis package)
	(package->license-string package)
	(package-home-page package)))

(define-public (pk-search regexps)
  (make-package-result
   (map first
	(sort-packages
	 (search-packages regexps)))))

(define-public (pk-resolve package-ids)
  (make-package-result
   (concatenate
    (map
     (lambda (requested-name)
       (let-values (((name version)
		     (package-name->name+version requested-name)))
	 (find-packages-by-name name version)))
     package-ids))))

(define get-package
  (compose packagekit-id->package string->packagekit-id))

(define-public (pk-get-details package-ids)
  (cond
   ((null? package-ids) '())
   (else
    (let ((package (get-package (car package-ids))))
      (if package
	  (cons (make-package-details-result package)
		(pk-get-details (cdr package-ids)))
	  (pk-get-details (cdr package-ids)))))))

(define-public (pk-install package-ids)
  (let ((packages (map get-package
		       package-ids)))
    (install-packages packages)))
