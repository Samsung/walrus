(module
  (func (export "block1")(param $num i32)(result i32)
    (block
      local.get 0
      i32.eqz
      br_if 0
      i32.const 42
      local.set 0)
    local.get 0
  )
  (func (export "block2")(param i32)(result i32)(local i32)
      (block
          (block
              (block
                   local.get 0
                   i32.eqz
                   br_if 0

                   local.get 0
                   i32.const 1
                   i32.eq
                   br_if 1

                   i32.const 7
                   local.set 1
                   br 2)
             i32.const 42
             local.set 1
             br 1)
         i32.const 99
         local.set 1)
       local.get 1)
)

(assert_return (invoke "block1" (i32.const 0)) (i32.const 0))
(assert_return (invoke "block1" (i32.const 1)) (i32.const 42))

(assert_return (invoke "block2" (i32.const 0)) (i32.const 42))
(assert_return (invoke "block2" (i32.const 1)) (i32.const 99))
(assert_return (invoke "block2" (i32.const 2)) (i32.const 7))
