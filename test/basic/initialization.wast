(module

(func (export "f1") (result i32)
  (local i32)

  i32.const 0
  (if 
    (then
      i32.const 1
      local.set 0 
    )
  )
  
  local.get 0

)

(func (export "f2") (result i32)
  (local i32)

  (loop $loop
    
    i32.const 1
    local.set 0

    i32.const 0
    br_if $loop
  )

  local.get 0
)

(func (export "f3") (result i32)
  (local i32)

  local.get 0
)

(func (export "f4") (result i32)
  (local i32)

  local.get 0
  i32.const 1
  i32.add
  local.tee 0
)


(func (export "f5") (result i32)
  (local i32 i32 i32)
  (block $while
    (loop $loop
      i32.const 1
      br_if $while

      i32.const 1
      local.set 0

      br $loop
    )
  )

  i32.const 1
  local.set 2
  (block $while
    (loop $loop
      local.get 2
      br_if $while

      local.get 0
      local.set 1

      i32.const 1
      local.get 2
      i32.sub
      local.set 2

      br $loop
    )
  )

  local.get 1
)

(func (export "f6") (param i32 ) (result i32)
  (local i32)

  (block
    (block
      (block
        local.get 0
        (br_table 0 1 2)
      )

      i32.const 1
      local.tee 1

      return
    )
    i32.const 2
    local.set 1
  )

  local.get 1
)

)

(assert_return (invoke "f1") (i32.const 0))
(assert_return (invoke "f2") (i32.const 1))
(assert_return (invoke "f3") (i32.const 0))
(assert_return (invoke "f4") (i32.const 1))
(assert_return (invoke "f5") (i32.const 0))
(assert_return (invoke "f6" (i32.const 0)) (i32.const 1))
(assert_return (invoke "f6" (i32.const 1)) (i32.const 2))
(assert_return (invoke "f6" (i32.const 2)) (i32.const 0))

