;; Regression for Samsung/escargot issue #1711.
;; https://github.com/Samsung/escargot/issues/1711
(module
  (type (;0;) (func (param v128)))
  (export "f" (func 0))
  (func (;0;) (type 0) (param v128))
)
