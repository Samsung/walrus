;; Regression for Samsung/escargot issue #1725.
;; https://github.com/Samsung/escargot/issues/1725
(module
  (type (;0;) (func (result i32 i32 i32)))
  (type (;1;) (func (param i32) (result i32 i32 i32)))
  (export "f" (func 0))
  (func (;0;) (type 0) (result i32 i32 i32)
    i32.const 100
    block (type 1) (param i32) (result i32 i32 i32) ;; label = @1
      drop
      i32.const 1
      i32.const 2
      i32.const 3
      br 0 (;@1;)
    end
  )
)
