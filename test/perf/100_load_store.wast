(module
(memory 1)

(func $100load-store
  (local i32)
  i32.const 0
  i32.const 42
  i32.store

  i32.const 0
  local.set 0
  (loop
    local.get 0
    i32.const 100
    i32.eq
    br_if 0

    local.get 0
    local.get 0
    i32.const 4
    i32.mul
    i32.store

    i32.const 1
    local.get 0
    i32.add
    local.set 0
  )

)

(func $100load
(local i32)
  i32.const 0
  i32.const 42
  i32.store

  i32.const 0
  local.set 0
  (loop
    local.get 0
    i32.const 100
    i32.eq
    br_if 0

    local.get 0
    i32.load offset=4
    drop

    i32.const 1
    local.get 0
    i32.add
    local.set 0
  )
)

(func $100store
(local i32)
  i32.const 0
  i32.const 42
  i32.store

  i32.const 0
  local.set 0
  (loop
    local.get 0
    i32.const 100
    i32.eq
    br_if 0

    local.get 0
    i32.const 42
    i32.store offset=4

    i32.const 1
    local.get 0
    i32.add
    local.set 0
  )
)

(func $start
  call $100load-store
  call $100load
  call $100store
)

(start $start)
)
