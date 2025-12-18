(module
    (memory $mem0 1)
    (memory $mem1 1)

    (func (export "storeload_f32") (param i32) (param f32) (result f32)
          (f32.store $mem0 offset=1 (i32.const 3) (local.get 1))
          (f32.load $mem0 offset=1 (i32.const 3))
    )
    (func (export "storeload_align_f32") (param i32) (param f32) (result f32)
          (f32.store $mem1 offset=1 (i32.const 0) (local.get 1))
          (f32.load $mem1 offset=1 (i32.const 0))
    )
)

(assert_return (invoke "storeload_f32" (i32.const 0) (f32.const 1234)) (f32.const 1234))
(assert_return (invoke "storeload_align_f32" (i32.const 0) (f32.const 1234)) (f32.const 1234))
