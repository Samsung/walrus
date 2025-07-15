(module
  (func $test (export "test")(param i32)(result i32)
  (local i32 i32 i32 i64)

  local.get 0 ;; start of 0
  local.get 1 ;; start of 1
  drop
  drop

  i32.const 32
  local.set 0

  i32.const 33
  local.set 1

  local.get 0
  local.get 1 ;; end of 1
  drop
  drop

  i32.const 34 
  local.set 0 ;; end of 0


  i32.const 1
  local.set 2 ;; start of 2
  local.get 2 ;; end of 2
  drop

  i64.const 23
  local.set 4
  local.get 4
  drop

  i32.const 0
  )


  (func $test2 (export "test2")(result i32)
  (local i32 i32 i32 i32 i32)

  i32.const 10
  local.set 0
  (loop $outer ;; runs 10 times
  
    i32.const 5
    local.set 1
    (loop $inner1 ;; runs 5 times
      i32.const 42
      local.set 2
      local.get 2
      drop

      local.get 1
      i32.const 1
      i32.sub
      local.tee 1

      i32.const 0
      i32.eq
      br_if $inner1
    )

    i32.const 8
    local.set 3
    (loop $inner2 ;; runs 8 times
      local.get 3
      i32.const 1
      i32.sub
      local.tee 3

      i32.const 0
      i32.eq
      br_if $inner2
    )

    local.get 0
    i32.const 1
    i32.sub
    local.tee 0

    i32.const 0
    i32.eq
    br_if $outer
  )

  (block $block
    i32.const 99999
    local.set 4

    i32.const 0
    i32.eqz
    br_if $block
    local.get 4

    ;;junk
    i32.const 0
    i32.add
    i32.eqz
    i32.clz
    drop
  )

  i32.const 0
  )
)

(assert_return (invoke "test" (i32.const 12))(i32.const 0))
(assert_return (invoke "test2")(i32.const 0))
