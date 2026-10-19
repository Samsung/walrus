;; Regression for Samsung/escargot issue #1717.
;; https://github.com/Samsung/escargot/issues/1717
(module
  (type (;0;) (func))
  (export "f" (func 0))
  (func (;0;) (type 0)
    ref.null exn
    throw_ref
  )
)
