;; Regression for Samsung/escargot issue #1719.
;; https://github.com/Samsung/escargot/issues/1719
(module
  (type (;0;) (func (param v128)))
  (func (;0;) (type 0) (param v128)
    local.get 0
    loop (type 0) (param v128) ;; label = @1
      drop
      i32.const 1
      local.get 0
      i32.const 0
      br_if 0 (;@1;)
      unreachable
    end
  )
)
