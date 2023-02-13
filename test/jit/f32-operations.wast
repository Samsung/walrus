(module
(func (export "test1") (result f32)
    f32.const 3.3
    f32.const 4.4
    f32.max
)
(func (export "test2") (result f32)
    f32.const 3.3
    f32.const 4.4
    f32.min
)
(func (export "test3") (result f32)
    f32.const 4.0
    f32.sqrt
)
(func (export "test4") (result f32)
    f32.const 3.3
    f32.ceil
)
(func (export "test5") (result f32)
    f32.const 3.3
    f32.floor
)
(func (export "test6") (result f32)
    f32.const 3.3
    f32.trunc
)
(func (export "test7") (result f32)
    f32.const 3.3
    f32.nearest
)
(func (export "test8") (result f32)
    f32.const 3.3
    f32.abs
)
(func (export "test9") (result f32)
    f32.const -3.3
    f32.abs
)
(func (export "test10") (result f32)
    f32.const -3.3
    f32.neg
)
)

(assert_return (invoke "test1") (f32.const 4.4))
(assert_return (invoke "test2") (f32.const 3.3))
(assert_return (invoke "test3") (f32.const 2.0))
(assert_return (invoke "test4") (f32.const 4.0))
(assert_return (invoke "test5") (f32.const 3.0))
(assert_return (invoke "test6") (f32.const 3.0))
(assert_return (invoke "test7") (f32.const 3.0))
(assert_return (invoke "test8") (f32.const 3.3))
(assert_return (invoke "test9") (f32.const 3.3))
(assert_return (invoke "test10") (f32.const 3.3))
