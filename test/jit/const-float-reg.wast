(module
;; A float constant stored into a local that another branch also writes
;; gets a float register. With many live f32 values it is a high one, which
;; must not be reserved in the smaller integer register set. The i32
;; constants keep the float constant out of the hoisted constant slots.
(func (export "const_high_float_reg") (param i32 f32 f32 f32 f32 f32 f32 f32 f32 f32 f32 f32 f32 f32 f32 f32) (result f32) (local f32)
  i32.const 1000
  i32.const 1000
  i32.add
  drop
  i32.const 1001
  i32.const 1001
  i32.add
  drop
  i32.const 1002
  i32.const 1002
  i32.add
  drop
  i32.const 1003
  i32.const 1003
  i32.add
  drop
  i32.const 1004
  i32.const 1004
  i32.add
  drop
  i32.const 1005
  i32.const 1005
  i32.add
  drop
  i32.const 1006
  i32.const 1006
  i32.add
  drop
  i32.const 1007
  i32.const 1007
  i32.add
  drop
  local.get 0
  if
    local.get 1
    local.get 2
    f32.div
    local.set 16
  else
    f32.const 0.25
    local.set 16
  end
  local.get 16
  local.get 1
  f32.add
  local.get 2
  f32.add
  local.get 3
  f32.add
  local.get 4
  f32.add
  local.get 5
  f32.add
  local.get 6
  f32.add
  local.get 7
  f32.add
  local.get 8
  f32.add
  local.get 9
  f32.add
  local.get 10
  f32.add
  local.get 11
  f32.add
  local.get 12
  f32.add
  local.get 13
  f32.add
  local.get 14
  f32.add
  local.get 15
  f32.add
)
)

(assert_return (invoke "const_high_float_reg" (i32.const 1) (f32.const 1) (f32.const 2) (f32.const 3) (f32.const 4) (f32.const 5) (f32.const 6) (f32.const 7) (f32.const 8) (f32.const 9) (f32.const 10) (f32.const 11) (f32.const 12) (f32.const 13) (f32.const 14) (f32.const 15)) (f32.const 120.5))
(assert_return (invoke "const_high_float_reg" (i32.const 0) (f32.const 1) (f32.const 2) (f32.const 3) (f32.const 4) (f32.const 5) (f32.const 6) (f32.const 7) (f32.const 8) (f32.const 9) (f32.const 10) (f32.const 11) (f32.const 12) (f32.const 13) (f32.const 14) (f32.const 15)) (f32.const 120.25))
