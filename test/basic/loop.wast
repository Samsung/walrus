(module
  (import "spectest" "print_i32" (func $log_i32 (param i32)))
  (func (export "loop1")(param $num i32)(result i32)(local $sum i32)
    (loop $loop
      local.get $num
      local.get $sum
      i32.add
      local.set $sum

      local.get $num
      i32.const 1
      i32.sub
      local.set $num

      local.get $num
      i32.eqz
      (if
        (then
        )
        (else
           br $loop
        )
      )
    )

    local.get $sum
  )

  (func (export "loop2")(param $num i32)(result i32)(local $sum i32)
    (loop $loop
      local.get $num
      local.get $sum
      i32.add
      local.set $sum

      local.get $num
      i32.const 1
      i32.sub
      local.set $num

      local.get $num
      br_if $loop
    )

    local.get $sum
  )

  (func (export "loop3")(param $num i32)(result i32)(local $sum i32)
    (loop $loop (result i32)
      local.get $num
      local.get $sum
      i32.add
      local.set $sum

      local.get $num
      i32.const 1
      i32.sub
      local.set $num

      local.get $num
      br_if $loop

      local.get $sum
    )
  )

  (func (export "loop4")(param $num i32)(result i32)
    (loop $outer_loop
      local.get $num
      i32.const 1
      i32.ge_s
      (if
        (then
          (loop $loop (result i32)
            local.get $num
            i32.const 1
            i32.sub
            local.set $num
            local.get $num
            local.get $num
            br_if $outer_loop
          )
          drop
        )
      )
    )
    local.get $num
  )
)

(assert_return (invoke "loop1" (i32.const 10)) (i32.const 55))
(assert_return (invoke "loop2" (i32.const 9)) (i32.const 45))
(assert_return (invoke "loop3" (i32.const 8)) (i32.const 36))
;; br, br_if should shrink stack correctly
(assert_return (invoke "loop4" (i32.const 100000)) (i32.const 0))

