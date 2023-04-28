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

(func (export "test11") (result i32 i32 i32 i32 i32 i32)
    f32.const 3.3
    f32.const 3.3
    f32.eq

    if (result i32)
        i32.const 69
    else
        i32.const 1
    end

    f32.const 3.3
    f32.const -3.3
    f32.ne

    if (result i32)
        i32.const 68
    else
        i32.const 2
    end

    f32.const 3.3
    f32.const 3.0
    f32.lt

    if (result i32)
        i32.const 3
    else
        i32.const 67
    end

    f32.const 4.0
    f32.const 4.3
    f32.gt

    if (result i32)
        i32.const 4
    else
        i32.const 66
    end

    f32.const 3.3
    f32.const 4.4
    f32.le

    if (result i32)
        i32.const 65
    else
        i32.const 5
    end

    f32.const 4.4
    f32.const 3.3
    f32.ge

    if (result i32)
        i32.const 64
    else
        i32.const 6
    end
)

(func (export "test12") (result i32)
    f32.const 3.3
    f32.const 3.3
    f32.eq
)

(func (export "test13") (result i32)
    f32.const 3.3
    f32.const 3.3
    f32.ne
)

(func (export "test14") (result i32 i32 i32)
    f32.const nan:0x200000
    f32.const inf
    f32.ge
    f32.const -inf
    f32.const -inf
    f32.ge
    f32.const -inf
    f32.const -0x0p+0
    f32.eq
)

(func (export "test15") (result i32 i32 i32 i32)
    f32.const -0x0p+0
    f32.const -nan
    f32.ne

    f32.const -0x0p+0
    f32.const -nan:0x200000
    f32.ne

    f32.const -0x1p-149
    f32.const -0x1p-149
    f32.ne

    f32.const 0x0p+0
    f32.const nan
    f32.ne
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
(assert_return (invoke "test11") (i32.const 69) (i32.const 68) (i32.const 67) (i32.const 66) (i32.const 65) (i32.const 64))
(assert_return (invoke "test12") (i32.const 1))
(assert_return (invoke "test13") (i32.const 0))
(assert_return (invoke "test14") (i32.const 0) (i32.const 1) (i32.const 0))
(assert_return (invoke "test15") (i32.const 1) (i32.const 1) (i32.const 0) (i32.const 1))
