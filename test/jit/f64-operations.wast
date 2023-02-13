(module
(func (export "test11") (result f64)
    f64.const 3.3
    f64.const 4.4
    f64.max
)
(func (export "test12") (result f64)
    f64.const 3.3
    f64.const 4.4
    f64.min
)
(func (export "test13") (result f64)
    f64.const 4.0
    f64.sqrt
)
(func (export "test14") (result f64)
    f64.const 3.3
    f64.ceil
)
(func (export "test15") (result f64)
    f64.const 3.3
    f64.floor
)
(func (export "test16") (result f64)
    f64.const 3.3
    f64.trunc
)
(func (export "test17") (result f64)
    f64.const 3.3
    f64.nearest
)
(func (export "test18") (result f64)
    f64.const 3.3
    f64.abs
)
(func (export "test19") (result f64)
    f64.const -3.3
    f64.abs
)
(func (export "test20") (result f64)
    f64.const -3.3
    f64.neg
)
(func (export "test21") (result f64)
    f64.const -3.3
    f64.const 3.3
    f64.copysign
)
)

(assert_return (invoke "test11") (f64.const 4.4))
(assert_return (invoke "test12") (f64.const 3.3))
(assert_return (invoke "test13") (f64.const 2.0))
(assert_return (invoke "test14") (f64.const 4.0))
(assert_return (invoke "test15") (f64.const 3.0))
(assert_return (invoke "test16") (f64.const 3.0))
(assert_return (invoke "test17") (f64.const 3.0))
(assert_return (invoke "test18") (f64.const 3.3))
(assert_return (invoke "test19") (f64.const 3.3))
(assert_return (invoke "test20") (f64.const 3.3))
(assert_return (invoke "test21") (f64.const 3.3))
