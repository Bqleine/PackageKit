(define-module (packagekit pk-guile-interface)
  #:use-module (gnu packages)
  #:use-module (guix packages)
  #:use-module (guix scripts show)
  #:use-module (guix utils)
  #:use-module (guix ui)
  #:use-module (ice-9 match))

(define-public (pk-show-package name)
  (guix-show name))

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

(define (make-package-result packages)
  (map (lambda (package-pair)
	 (let ((package (car package-pair)))
	   (cons
	    (string-append (package-name package)
			   ";"
			   (package-version package)
			   ";"
			   ";")
	    (package-description package))))
       packages))

(define-public (pk-search regexps)
  (make-package-result
   (sort-packages
    (search-packages regexps))))
