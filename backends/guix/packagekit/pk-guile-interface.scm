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
  #:use-module ((gnu packages) #:select (find-packages-by-name))
  #:use-module ((guix licenses) #:select (license-name license?))
  #:use-module ((guix utils) #:select (package-name->name+version))
  #:use-module (guix packages)
  #:use-module (packagekit pk-id)
  #:use-module (packagekit pk-profile)
  #:use-module (packagekit pk-query)
  #:use-module (srfi srfi-1)
  #:use-module (srfi srfi-11))

(define (make-package-result packages)
  (map (lambda (package)
	 (cons
	  (packagekit-id->string (package->packagekit-id package))
	  (package-description package)))
       packages))

(define (package-license-string package)
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
  (list (packagekit-id->string (package->packagekit-id package))
	(package-description package)
	(package-synopsis package)
	(package-license-string package)
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

(define-public (pk-get-details package-ids)
  (define get-package
    (compose packagekit-id->package string->packagekit-id))

  (cond
   ((null? package-ids) '())
   (else
    (let ((package (get-package (car package-ids))))
      (if package
	  (cons (make-package-details-result package)
		(pk-get-details (cdr package-ids)))
	  (pk-get-details (cdr package-ids)))))))

(define-public (pk-install package-ids)
  (profile-install (map string->packagekit-id package-ids)))

(define-public (pk-remove package-ids)
  (profile-remove (map string->packagekit-id package-ids)))

(define-public (pk-upgrade package-ids)
  (profile-upgrade (map string->packagekit-id package-ids)))
