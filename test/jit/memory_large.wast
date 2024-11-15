(module
    (; 1GB memory, can be quite large for a 32 bit system. ;)
    (memory 16384 16384)

    (func (export "set") (param i32 i32)
        local.get 0
        local.get 1
        i32.store8
    )

    (func (export "get") (param i32) (result i32)
        local.get 0
        i32.load8_u
    )
)

(invoke "set" (i32.const 0x3fffffff) (i32.const 0xaa))
(assert_return (invoke "get" (i32.const 0x3fffffff)) (i32.const 0xaa))
(assert_trap (invoke "get" (i32.const 0x40000000)) "out of bounds memory access")
