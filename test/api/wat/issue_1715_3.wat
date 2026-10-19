;; Regression for Samsung/escargot issue #1715.
;; https://github.com/Samsung/escargot/issues/1715
(module
  (type (;0;) (func))
  (func (;0;) (type 0)
    i32.const 0
    drop
    block ;; label = @1
      br 0 (;@1;)
      try_table ;; label = @2
      end
      drop
    end
  )
)
