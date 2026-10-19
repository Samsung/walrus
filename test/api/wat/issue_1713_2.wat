;; Regression for Samsung/escargot issue #1713.
;; https://github.com/Samsung/escargot/issues/1713
(module
  (type (;0;) (func))
  (func (;0;) (type 0)
    try ;; label = @1
    catch_all
    end
  )
)
