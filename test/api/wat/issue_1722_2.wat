;; Regression for Samsung/escargot issue #1722.
;; https://github.com/Samsung/escargot/issues/1722
(module
  (type (;0;) (func (param i64) (result i32 i32)))
  (type (;1;) (func))
  (tag (;0;) (type 1))
  (export "f" (func 0))
  (func (;0;) (type 0) (param i64) (result i32 i32)
    local.get 0
    try (type 0) (param i64) (result i32 i32) ;; label = @1
      drop
      throw 0
    catch_all
      i32.const 7
      i32.const 9
    end
  )
)
