;; Regression for Samsung/escargot issue #1721.
;; https://github.com/Samsung/escargot/issues/1721
(module
  (type (;0;) (func (param i32) (result i32)))
  (memory (;0;) 1)
  (export "f" (func 0))
  (func (;0;) (type 0) (param i32) (result i32)
    i32.const 1
    local.get 0
    i32.eqz
    memory.atomic.notify
  )
)
