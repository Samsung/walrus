;; Regression for Samsung/escargot issue #1715.
;; https://github.com/Samsung/escargot/issues/1715
(module
  (type (;0;) (func (result i32)))
  (export "f" (func 0))
  (func (;0;) (type 0) (result i32)
    block (result i32) ;; label = @1
      i32.const 5
      br 0 (;@1;)
      try_table ;; label = @2
      end
      unreachable
    end
  )
)
