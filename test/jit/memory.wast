(module
    (memory 1)
    (func (export "storeload_f32") (param i32) (param f32) (result f32)
          (f32.store offset=1 (i32.const 0) (local.get 1))
          (f32.load offset=1 (i32.const 0))
    )
)

(assert_return (invoke "storeload_f32" (i32.const 0) (f32.const 1234)) (f32.const 1234))

(module
  (memory 16 16)
  (export "main" (func 0))

  (func (export "mem_init")
    i32.const 0
    i32.const 1
    i32.const -1
    memory.init 0
  )
  (data (;0;) (i32.const 0) "\01\02\03\04\05\06\07\08\09\0a\0b\0c\0d\0e\0f\10")
)

(assert_trap (invoke "mem_init") "out of bounds memory access")
