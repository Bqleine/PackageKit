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

(define-module (packagekit pk-id)
  #:use-module (srfi srfi-9)
  #:use-module (ice-9 match)
  #:use-module ((gnu packages) #:select (find-packages-by-name)))

(define-record-type <packagekit-id>
  (make-packagekit-id name version arch data)
  packagekit-id?
  (name packagekit-id-name)
  (version packagekit-id-version)
  (arch packagekit-id-arch)
  (data packagekit-id-data))

(define-public (string->packagekit-id id)
  (unless (eq? (string-count id #\;) 3)
    (error "packagekit id should always contain four elements"))
  (define components (string-split id #\;))
  (match-let (((name version arch data) components))
    (make-packagekit-id name version arch data)))

(define-public (packagekit-id->string pk-id)
  (string-append
   (packagekit-id-name pk-id)
   ";"
   (packagekit-id-version pk-id)
   ";"
   (packagekit-id-arch pk-id)
   ";"
   (packagekit-id-data pk-id)))

(define-public (packagekit-id->package pk-id)
  (let* ((name (packagekit-id-name pk-id))
	 (version (packagekit-id-version pk-id))
	 (packages (find-packages-by-name name version)))
    (if (null? packages) '() (car packages))))

(define-public (packagekit-id->guix-id pk-id)
  (let ((name (packagekit-id-name pk-id))
	(version (packagekit-id-version pk-id)))
    (if (string-null? version)
	name
	(string-append name "@" version))))
