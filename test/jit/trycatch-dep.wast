(module
  (memory 1)

  (tag $except0 (param i32 i64))
  (tag $except1 (param f32 f64))
  (tag $except3)

  (func $throw1 (param i64)
     i32.const 1234
     local.get 0
     throw $except0
  )

  (func $throw2 (param i64)
     f32.const 2345.0
     local.get 0
     f64.convert_i64_s
     throw $except1
  )

  (func $throw3 (param i64)
     throw $except3
  )

  (func (export "try1") (param i64) (result i64 i32) (local $l1 i64) (local $l2 i32)
    i32.const 123456
    local.set $l2
    (try
      (do
        (try
          (do
            local.get 0
            local.set $l1

            local.get $l1
            i32.wrap_i64
            i32.const 100
            i32.add
            local.set $l2

            i64.const 1000
            local.get $l1
            i64.lt_s
            if
              local.get $l1
              call $throw1
            end

            local.get $l1
            i64.const 500
            i64.gt_s
            if
              local.get $l1
              call $throw2
            end

            local.get $l1
            i64.const 250
            i64.gt_s
            if
              local.get $l1
              call $throw3
            end

            local.get $l1
            i64.const 17
            i64.sub
            i32.const -678
            return
          )
          (catch $except0
            local.set $l1
            i64.extend_i32_s
            local.get $l1
            i64.add
            local.set $l1
          )
        )
      )
      (catch $except1
        i64.trunc_sat_f64_s
        local.set $l1

        i64.trunc_sat_f32_s
        local.get $l1
        i64.add
        local.set $l1
      )
      (catch_all
        i64.const 888
        local.set $l1
      )
    )

    local.get $l1
    local.get $l2
  )
)

(assert_return (invoke "try1" (i64.const 99)) (i64.const 82) (i32.const -678))
(assert_return (invoke "try1" (i64.const 2000)) (i64.const 3234) (i32.const 2100))
(assert_return (invoke "try1" (i64.const 600)) (i64.const 2945) (i32.const 700))
(assert_return (invoke "try1" (i64.const 300)) (i64.const 888) (i32.const 400))
