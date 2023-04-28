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

(func (export "test22") (result i32 i32 i32 i32 i32 i32)
    f64.const 3.3
    f64.const 3.3
    f64.eq

    if (result i32)
        i32.const 69
    else
        i32.const 1
    end

    f64.const 3.3
    f64.const -3.3
    f64.ne

    if (result i32)
        i32.const 68
    else
        i32.const 2
    end

    f64.const 3.3
    f64.const 3.0
    f64.lt

    if (result i32)
        i32.const 3
    else
        i32.const 67
    end

    f64.const 4.0
    f64.const 4.3
    f64.gt

    if (result i32)
        i32.const 4
    else
        i32.const 66
    end

    f64.const 3.3
    f64.const 4.4
    f64.le

    if (result i32)
        i32.const 65
    else
        i32.const 5
    end

    f64.const 4.4
    f64.const 3.3
    f64.ge

    if (result i32)
        i32.const 64
    else
        i32.const 6
    end
)

(func (export "test23") (result i32)
    f64.const 3.3
    f64.const 3.3
    f64.eq
)

(func (export "test24") (result i32)
    f64.const 3.3
    f64.const 3.3
    f64.ne
)

(func (export "test25") (result i32 i32 i32)
    f64.const nan:0x200000
    f64.const inf
    f64.ge
    f64.const -inf
    f64.const -inf
    f64.ge
    f64.const -inf
    f64.const -0x0p+0
    f64.eq
)

(func (export "test26") (result i32 i32 i32 i32)
    f64.const -0x0p+0
    f64.const -nan
    f64.ne

    f64.const -0x0p+0
    f64.const -nan:0x200000
    f64.ne

    f64.const -0x1p-149
    f64.const -0x1p-149
    f64.ne

    f64.const 0x0p+0
    f64.const nan
    f64.ne
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
(assert_return (invoke "test22") (i32.const 69) (i32.const 68) (i32.const 67) (i32.const 66) (i32.const 65) (i32.const 64))
(assert_return (invoke "test23") (i32.const 1))
(assert_return (invoke "test24") (i32.const 0))
(assert_return (invoke "test25") (i32.const 0) (i32.const 1) (i32.const 0))
(assert_return (invoke "test26") (i32.const 1) (i32.const 1) (i32.const 0) (i32.const 1))
