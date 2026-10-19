;; Regression for Samsung/escargot issue #1710.
;; https://github.com/Samsung/escargot/issues/1710
;; Foreign reference round trip
(module
  (type (;0;) (func (param externref) (result externref)))
  (export "id" (func 0))
  (func (;0;) (type 0) (param externref) (result externref)
    local.get 0
  )
)
