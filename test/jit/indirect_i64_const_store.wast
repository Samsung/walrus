(module
 (memory 1 1)
 ;; regression test for 32bit JIT
 (func (export "i64_indirect_store_const") (param i32)
    (i64.store
        (local.get 0)
        (i64.const 0xcafebabe)
    )
 )
 (func (export "i64_load") (param i32) (result i64)
    (i64.load (local.get 0))
 )
)
(invoke "i64_indirect_store_const" (i32.const 10))
(assert_return (invoke "i64_load" (i32.const 10)) (i64.const 0xcafebabe))
