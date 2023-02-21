(module
(memory 1)
;; stores
(func (export "64store") (param i32 i64)
  local.get 0
  local.get 1
  i64.store
)
(func (export "64store32") (param i32 i64)
  local.get 0
  local.get 1
  i64.store32
)
(func (export "64store16") (param i32 i64)
  local.get 0
  local.get 1
  i64.store16
)
(func (export "64store8") (param i32 i64)
  local.get 0
  local.get 1
  i64.store8
)
(func (export "32store") (param i32 i32)
  local.get 0
  local.get 1
  i32.store
)
(func (export "32store16") (param i32 i32)
  local.get 0
  local.get 1
  i32.store16
)
(func (export "32store8") (param i32 i32)
  local.get 0
  local.get 1
  i32.store8
)
;; loads
(func (export "64load") (param i32) (result i64)
  local.get 0
  i64.load
)
(func (export "64load32s") (param i32) (result i64)
  local.get 0
  i64.load32_s
)
(func (export "64load32u") (param i32) (result i64)
  local.get 0
  i64.load32_u
)
(func (export "64load16s") (param i32) (result i64)
  local.get 0
  i64.load16_s
)
(func (export "64load16u") (param i32) (result i64)
  local.get 0
  i64.load16_u
)
(func (export "64load8s") (param i32) (result i64)
  local.get 0
  i64.load8_s
)
(func (export "64load8u") (param i32) (result i64)
  local.get 0
  i64.load8_u
)
(func (export "32load") (param i32) (result i32)
  local.get 0
  i32.load
)
(func (export "32load16s") (param i32) (result i32)
  local.get 0
  i32.load16_s
)
(func (export "32load16u") (param i32) (result i32)
  local.get 0
  i32.load16_u
)
(func (export "32load8s") (param i32) (result i32)
  local.get 0
  i32.load8_s
)
(func (export "32load8u") (param i32) (result i32)
  local.get 0
  i32.load8_u
)
(func (export "8offsetStore") (param i32 i64)
  local.get 0
  local.get 1
  i64.store offset=8
)
(func (export "8offsetLoad") (param i32) (result i64)
  local.get 0
  i64.load
)

(func (export "maxOffsetStore") (param i32 i64)
  local.get 0
  local.get 1
  i64.store offset=0xFFFFFFFF
)

(func (export "maxOffsetLoad") (param i32) (result i64)
  local.get 0
  i64.load offset=0xFFFFFFFF
)
)

(assert_return (invoke "64store" (i32.const 0) (i64.const 0x8000000000000000)))
(assert_return (invoke "64load" (i32.const 0)) (i64.const 0x8000000000000000))
(assert_return (invoke "64store32" (i32.const 8) (i64.const 0x180000000)))
(assert_return (invoke "64load32s" (i32.const 8)) (i64.const -0x80000000))
(assert_return (invoke "64load32u" (i32.const 8)) (i64.const 0x80000000))
(assert_return (invoke "64store16" (i32.const 16) (i64.const 0x18000)))
(assert_return (invoke "64load16s" (i32.const 16)) (i64.const 0xFFFFFFFFFFFF8000))
(assert_return (invoke "64load16u" (i32.const 16)) (i64.const 0x8000))
(assert_return (invoke "64store8" (i32.const 24) (i64.const 0x180)))
(assert_return (invoke "64load8s" (i32.const 24)) (i64.const 0xFFFFFFFFFFFFFF80))
(assert_return (invoke "64load8u" (i32.const 24)) (i64.const 0x80))
(assert_return (invoke "32store" (i32.const 32) (i32.const 0x80000000)))
(assert_return (invoke "32load" (i32.const 32)) (i32.const 0x80000000))
(assert_return (invoke "32store16" (i32.const 36) (i32.const 0x18000)))
(assert_return (invoke "32load16s" (i32.const 36)) (i32.const 0xFFFF8000))
(assert_return (invoke "32load16u" (i32.const 36)) (i32.const 0x8000))
(assert_return (invoke "32store8" (i32.const  40) (i32.const 0x180)))
(assert_return (invoke "32load8s" (i32.const 40)) (i32.const 0xFFFFFF80))
(assert_return (invoke "32load8u" (i32.const 40)) (i32.const 0x80))
(assert_return (invoke "32store8" (i32.const 40) (i32.const 0x12A)))
(assert_return (invoke "32load8s" (i32.const 40)) (i32.const 0x2A))
(assert_return (invoke "32load8u" (i32.const 40)) (i32.const 0x2A))
(assert_return (invoke "8offsetStore" (i32.const 44) (i64.const 0x2A)))
(assert_return (invoke "8offsetLoad" (i32.const 52)) (i64.const 0x2A))

(assert_trap (invoke "64store" (i32.const 65536) (i64.const 42)) "out of bounds memory access")
(assert_trap (invoke "64store" (i32.const 65529) (i64.const 42)) "out of bounds memory access")
(assert_trap (invoke "64load" (i32.const 65536)) "out of bounds memory access")
(assert_trap (invoke "64load" (i32.const 65529)) "out of bounds memory access")
(assert_trap (invoke "32store" (i32.const 65536) (i64.const 42)) "out of bounds memory access")
(assert_trap (invoke "32store" (i32.const 65533) (i64.const 42)) "out of bounds memory access")
(assert_trap (invoke "32load" (i32.const 65536)) "out of bounds memory access")
(assert_trap (invoke "32load" (i32.const 65533)) "out of bounds memory access")
(assert_trap (invoke "maxOffsetStore" (i32.const 0) (i32.const 0xdeadc0de)) "out of bounds memory access")
(assert_trap (invoke "maxOffsetLoad" (i32.const 0)) "out of bounds memory access")
