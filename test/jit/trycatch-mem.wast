(module
  (memory 1)
  (tag $except0 (param i32 i64 i32))

  (func $throw1 (param i64 i32)
     local.get 1
     memory.grow

     local.get 0
     i64.const 0xffffffffffff
     i64.add

     memory.size
     throw $except0
  )

  (func (export "try1") (result i32 i64 i32 i32 i32)
    (try (result i32 i64 i32 i32 i32)
      (do
         i64.const 0x123456781234
         i32.const 1
         call $throw1

         i32.const 0
         i64.const 0
         i32.const 0
         i32.const 0
         i32.const 0
      )
      (catch $except0
         i32.const 2
         memory.grow
         memory.size
      )
    )
  )
)

(assert_return (invoke "try1") (i32.const 1) (i64.const 0x1123456781233) (i32.const 2) (i32.const 2) (i32.const 4))
