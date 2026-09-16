(module
  (memory 1)

  (func (export "store_self") (param $p i32)
    (i32.store offset=16 (local.get $p) (local.get $p))
  )

  (func (export "store_self64") (param $p i32)
    (i64.store offset=32 (local.get $p) (i64.extend_i32_u (local.get $p)))
  )

  (func (export "load_it") (param $p i32) (result i32)
    (i32.load offset=16 (local.get $p))
  )

  (func (export "load_it64") (param $p i32) (result i64)
    (i64.load offset=32 (local.get $p))
  )
)

(assert_return (invoke "store_self" (i32.const 32)))
(assert_return (invoke "load_it" (i32.const 32)) (i32.const 32))
(assert_return (invoke "store_self64" (i32.const 64)))
(assert_return (invoke "load_it64" (i32.const 64)) (i64.const 64))
