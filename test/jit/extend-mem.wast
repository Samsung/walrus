(module
(func $nop)

;; Live across calls, more values than callee-saved registers, so some
;; extend results get a stack slot that still holds an older i64 value.
(func (export "extend_u") (param i32) (result i64)
  (local i64 i64 i64 i64 i64 i64)
  i64.const -1 local.set 1
  i64.const -1 local.set 2
  i64.const -1 local.set 3
  i64.const -1 local.set 4
  i64.const -1 local.set 5
  i64.const -1 local.set 6
  call $nop
  local.get 0 i32.const 1 i32.add i64.extend_i32_u local.set 1
  local.get 0 i32.const 2 i32.add i64.extend_i32_u local.set 2
  local.get 0 i32.const 3 i32.add i64.extend_i32_u local.set 3
  local.get 0 i32.const 4 i32.add i64.extend_i32_u local.set 4
  local.get 0 i32.const 5 i32.add i64.extend_i32_u local.set 5
  local.get 0 i32.const 6 i32.add i64.extend_i32_u local.set 6
  call $nop
  local.get 1 local.get 2 i64.add
  local.get 3 i64.add local.get 4 i64.add
  local.get 5 i64.add local.get 6 i64.add
)

(func (export "extend_s") (param i32) (result i64)
  (local i64 i64 i64 i64 i64 i64)
  i64.const 0x7fffffff00000000 local.set 1
  i64.const 0x7fffffff00000000 local.set 2
  i64.const 0x7fffffff00000000 local.set 3
  i64.const 0x7fffffff00000000 local.set 4
  i64.const 0x7fffffff00000000 local.set 5
  i64.const 0x7fffffff00000000 local.set 6
  call $nop
  local.get 0 i32.const 1 i32.add i64.extend_i32_s local.set 1
  local.get 0 i32.const 2 i32.add i64.extend_i32_s local.set 2
  local.get 0 i32.const 3 i32.add i64.extend_i32_s local.set 3
  local.get 0 i32.const 4 i32.add i64.extend_i32_s local.set 4
  local.get 0 i32.const 5 i32.add i64.extend_i32_s local.set 5
  local.get 0 i32.const 6 i32.add i64.extend_i32_s local.set 6
  call $nop
  local.get 1 local.get 2 i64.add
  local.get 3 i64.add local.get 4 i64.add
  local.get 5 i64.add local.get 6 i64.add
)
)

(assert_return (invoke "extend_u" (i32.const 10)) (i64.const 81))
(assert_return (invoke "extend_s" (i32.const 10)) (i64.const 81))
