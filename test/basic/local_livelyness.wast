(module
  (func $local_zero (export "local_zero")(result i32)
    (local i32)
    local.get 0
  )

  (func $local_loop1 (export "local_loop1")(result i32)
    (local i32 i32 i32)
    i32.const 10
    local.set 0 ;;start of 0

    ;;start of 1
    (loop $loop
      i32.const 1
      local.set 1 ;;start of 1, but inside loop
    
      local.get 0
      i32.const 1
      i32.sub
      local.tee 0
      i32.eqz
      br_if $loop
    )

    local.get 1 ;;end of 1
  )

  (func $local_blocks (export "local_block1")(result i32)
    (local i32 i32 i32 i32 i64)

    ;;end of 2

    local.get 4 ;; start of 4
    local.get 3 ;; start of 3
    drop
    drop

    i32.const 0
    local.set 0 ;; start of 0


    (block $block1
      i32.const 1
      local.get 0
      i32.add
      local.set 0

      (loop $block2
        local.get 1 ;; start of 1
        i32.const 3
        i32.eq
        br_if $block2
      )

      i32.const 0
      local.get 1
      i32.add
      drop
    ) ;; end of 1

  ;; end of 3, 4
  i32.const 0
  )

  (func $local_blocks2 (export "local_block2")(param i32)(result i32)
    (local i32)

    i32.const 1
    i32.const 1
    i32.sub
    drop
    
    local.get 0
    local.tee 1
  )

  (func $params (export "params")(param i32 i64 i32 v128)(result i32)
    (local i32 i64 v128)
    i32.const 0
  )

  (func $params2 (export "params2")(param v128 i32 v128)(result i32)
    i32.const 0
  )


  (func $params3 (export "params3")(param v128 i32 v128)
    i32.const 0
    local.set 1
    v128.const i64x2 0 0
    local.set 0
  )

)

(assert_return (invoke "local_zero") (i32.const 0))
(assert_return (invoke "local_loop1") (i32.const 1))
(assert_return (invoke "local_block1") (i32.const 0))
(assert_return (invoke "local_block2" (i32.const 42)) (i32.const 42))
(assert_return (invoke "params" (i32.const 1) (i64.const 2) (i32.const 3) (v128.const i64x2 4 5)) (i32.const 0))
(assert_return (invoke "params2" (v128.const i64x2 1 2) (i32.const 3) (v128.const i64x2 4 5)) (i32.const 0))
(assert_return (invoke "params3" (v128.const i64x2 1 2) (i32.const 3) (v128.const i64x2 4 5)))
