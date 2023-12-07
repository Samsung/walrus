(module
    (memory 1)
    (func (export "storeload_f32") (param i32) (param f32) (result f32)
          (f32.store offset=1 (i32.const 0) (local.get 1))
          (f32.load offset=1 (i32.const 0))
    )
)

(assert_return (invoke "storeload_f32" (i32.const 0) (f32.const 1234)) (f32.const 1234))
