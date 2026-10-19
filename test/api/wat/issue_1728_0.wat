;; Regression for Samsung/escargot issue #1728.
;; https://github.com/Samsung/escargot/issues/1728
(module
  (type (;0;) (func))
  (type (;1;) (func (param i32 v128 v128 f64 f32 f64)))
  (type (;2;) (func (param f64 i32 i32 f32) (result f32)))
  (type (;3;) (func (param f32 f64) (result f64)))
  (memory (;0;) 1)
  (export "w0" (func 0))
  (func (;0;) (type 3) (param f32 f64) (result f64)
    (local f64 f64 v128 v128 f32 f64 v128 v128 f64 i32 v128 f32)
    local.get 10
    local.set 2
    local.get 11
    local.get 12
    local.get 12
    local.get 10
    local.get 13
    local.get 10
    loop (type 1) (param i32 v128 v128 f64 f32 f64) ;; label = @1
      local.set 3
      drop
      drop
      local.set 4
      local.set 5
      drop
      local.get 3
      local.get 11
      local.get 11
      local.get 0
      local.get 11
      if (type 2) (param f64 i32 i32 f32) (result f32) ;; label = @2
        drop
        drop
        drop
        drop
        local.get 13
      else
        drop
        drop
        drop
        drop
        local.get 13
      end
      drop
      local.get 11
      local.get 12
      local.get 12
      local.get 10
      local.get 13
      local.get 1
      local.get 11
      br_if 0 (;@1;)
      drop
      local.set 6
      local.set 7
      local.set 8
      local.set 9
      drop
    end
    local.get 2
  )
)
