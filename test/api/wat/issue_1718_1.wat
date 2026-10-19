;; Regression for Samsung/escargot issue #1718.
;; https://github.com/Samsung/escargot/issues/1718
(module
  (type (;0;) (func (param i64 i64 i64 i64) (result i64 i64)))
  (func (;0;) (type 0) (param i64 i64 i64 i64) (result i64 i64)
    local.get 0
    local.get 1
    i64.add
    local.get 2
    local.get 3
    i64.add
  )
)
