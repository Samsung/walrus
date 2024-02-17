;; tests for Walrus where optimalisation shouldn't occure

(module

  ;; in the following tests there will be an EqualZero, then
  ;; a JumpIfTrue/JumpIfFalse that should not be unified

  (func (export "test1") (param $input i32) (result i32)
    i32.const 1
    i32.eqz
    local.get $input
    br_if 0
    drop
    i32.const 1
  )

  (func (export "test2") (param $input i32) (result i32)
    local.get $input
    i32.eqz
    i32.const 0
    if (result i32)
      i32.const 1
    else
      i32.const 0
    end
    i32.add
  )

  (func (export "test3") (param $input i32) (result i32)
    i32.const 1
    block (result i32)
      i32.const 0
      local.get $input
      br_if 0
      drop
      i32.const 0
      i32.eqz
    end
    br_if 0
    drop
    i32.const 0
  )
)

(assert_return (invoke "test1" (i32.const 0)) (i32.const 1))
(assert_return (invoke "test1" (i32.const 1)) (i32.const 0))
(assert_return (invoke "test2" (i32.const 0)) (i32.const 1))
(assert_return (invoke "test2" (i32.const 1)) (i32.const 0))
(assert_return (invoke "test3" (i32.const 0)) (i32.const 1))
(assert_return (invoke "test3" (i32.const 1)) (i32.const 0))
