(module
  (func (export "ifelse1")(param $num i32)(result i32)
    local.get $num
    i32.const 100
    i32.eq

    (if
      (then
        i32.const 123
        local.set $num
      )
      (else
        i32.const 456
        local.set $num
      )
    )

    local.get $num
  )

  (func (export "ifelse2")(param $num i32)(result i32)
    local.get $num
    i32.const 100
    i32.eq

    (if
      (then
        i32.const 123
        local.set $num
      )
      (else
        i32.const 456
        local.set $num
      )
    )

    local.get $num
  )


  (func (export "ifelse3")(param $num i32)(result i32)
    local.get $num
    i32.const 100
    i32.eq

    (if (result i32)
      (then
        i32.const 123
      )
      (else
        i32.const 456
      )
    )
  )
)

(assert_return (invoke "ifelse1" (i32.const 100)) (i32.const 123))
(assert_return (invoke "ifelse1" (i32.const 200)) (i32.const 456))

(assert_return (invoke "ifelse2" (i32.const 100)) (i32.const 123))
(assert_return (invoke "ifelse2" (i32.const 200)) (i32.const 456))

(assert_return (invoke "ifelse3" (i32.const 100)) (i32.const 123))
(assert_return (invoke "ifelse3" (i32.const 200)) (i32.const 456))


