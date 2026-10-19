;; Regression for Samsung/escargot issue #1722.
;; https://github.com/Samsung/escargot/issues/1722
(module
  (type (;0;) (func (param i32) (result i32)))
  (type (;1;) (func (result v128 i32)))
  (func (;0;) (type 0) (param i32) (result i32)
    try (type 1) (result v128 i32) ;; label = @1
      unreachable
    catch_all
      v128.const i32x4 0x00000000 0x00000000 0x00000000 0x00000000
      local.get 0
    end
    return
  )
)
