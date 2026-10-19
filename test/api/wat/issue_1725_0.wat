;; Regression for Samsung/escargot issue #1725.
;; https://github.com/Samsung/escargot/issues/1725
(module
  (type (;0;) (func))
  (type (;1;) (func (param i32) (result i32 i32 i32)))
  (func (;0;) (type 0)
    i32.const 0
    block (type 1) (param i32) (result i32 i32 i32) ;; label = @1
      drop
      i32.const 0
      i32.const 0
      i32.const 0
      br 0 (;@1;)
    end
    drop
    drop
    drop
  )
)
