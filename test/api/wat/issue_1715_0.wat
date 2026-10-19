;; Regression for Samsung/escargot issue #1715.
;; https://github.com/Samsung/escargot/issues/1715
(module
  (type (;0;) (func (result i32)))
  (func (;0;) (type 0) (result i32)
    block (result i32) ;; label = @1
      i32.const 1
      if ;; label = @2
      else
        unreachable
        try_table ;; label = @3
        end
      end
      i32.const 1
    end
  )
)
