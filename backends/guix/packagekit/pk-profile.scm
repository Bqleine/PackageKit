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

(define-module (packagekit pk-profile)
  #:use-module ((guix scripts package) #:select (guix-package* %package-default-options))
  #:use-module ((packagekit pk-id) #:select (packagekit-id->guix-id)))

;; Manual way, installing with a list of package records.  Currently
;; disabled for the other way below.

'(
 (define (package->manifest-entry* package output)
   "Like 'package->manifest-entry', but attach PACKAGE provenance meta-data to
the resulting manifest entry."
   (manifest-entry-with-provenance
    (package->manifest-entry package output)))

 (define (packages->manifest-entries packages)
   (map (lambda (package)
	  (package->manifest-entry* package "out"))
	packages))

 (define-public (install-packages packages)
   (let* ((profile %current-profile)
	  (manifest (profile-manifest profile))
	  (store (open-connection))
	  (entries (packages->manifest-entries packages))
	  (new-manifest (manifest-add manifest entries)))
     (display "INSTALLING TO PROFILE ")
     (display profile)
     (newline)
     (with-profile-lock profile
       (build-and-use-profile store profile new-manifest))))
 )

;; CLI way, calling guix-package* from the guix package command.  This
;; is not ideal but its the most stable way as long as cli and profile
;; actions are intertwined on the Guix side.

(define (guix-package-action pk-ids action)
  "Takes a list of <packagekit-id> and executes ACTION, a symbol
describing a guix package action, on them in the current profile."
  (let* ((actions
	  (map (lambda (pk-id)
		 (cons action (packagekit-id->guix-id pk-id)))
	       pk-ids)))
    (guix-package* `(,@actions
		     ,@%package-default-options))))

(define-public (profile-install pk-ids)
  (guix-package-action pk-ids 'install))

(define-public (profile-remove pk-ids)
  (guix-package-action pk-ids 'remove))

(define-public (profile-upgrade pk-ids)
  (guix-package-action pk-ids 'upgrade))
