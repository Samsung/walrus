;; Regression for Samsung/escargot issue #1718.
;; https://github.com/Samsung/escargot/issues/1718
(module
  (type (;0;) (func))
  (func (;0;) (type 0)
    i64.const 0
    i64.const 0
    i64.const 0
    i64.const 0
    i64.add128
    drop
    drop
  )
)
