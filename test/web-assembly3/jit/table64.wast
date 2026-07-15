(module
  (table $t i64 10 12 funcref)
  (memory $m 1)
  (global $g i32 (i32.const 1234))

  (elem (table $t) (i64.const 1) func $dummy)
  (elem (table $t) (i64.const 3) func $dummy)
  (elem funcref (ref.null func) (ref.null func) (ref.func $dummy))
  (func $dummy)

  (func (export "set1") (param i64) (result i32)
    (table.set $t (local.get 0) (ref.func $dummy))
    (i32.load $m (i32.const 0))
  )

  (func (export "set2") (param i64) (result i32)
    (table.set $t (local.get 0) (ref.func $dummy))
    (table.set $t (local.get 0) (ref.func $dummy))
    (i32.load $m (i32.const 0))
  )

  (func (export "get1") (param i64) (result funcref i32)
    (table.get $t (local.get 0))
    (i32.load $m (i32.const 0))
  )

  (func (export "get2") (param i64) (result funcref funcref i32)
    (table.get $t (local.get 0))
    (table.get $t (local.get 0))
    (global.get $g)
  )

  (func (export "grow") (param i64) (result i64 i32)
    (table.grow (ref.func $dummy) (local.get 0))
    (global.get $g)
  )

  (func (export "fill") (param i64 i64) (result i32)
    (table.fill (local.get 0) (ref.func $dummy) (local.get 1))
    (global.get $g)
  )

  (func (export "init") (param i64 i32 i32) (result i32)
    (table.init $t 2 (local.get 0) (local.get 1) (local.get 2))
    (global.get $g)
  )

  (func (export "copy") (param i64 i64 i64) (result i32)
    (table.copy $t $t (local.get 0) (local.get 1) (local.get 2))
    (global.get $g)
  )
)

(assert_return (invoke "set1" (i64.const 5)) (i32.const 0))
(assert_return (invoke "set2" (i64.const 7)) (i32.const 0))

(assert_return (invoke "get1" (i64.const 0)) (ref.null func) (i32.const 0))
(assert_return (invoke "get1" (i64.const 1)) (ref.func) (i32.const 0))
(assert_return (invoke "get2" (i64.const 2)) (ref.null func) (ref.null func) (i32.const 1234))
(assert_return (invoke "get2" (i64.const 1)) (ref.func) (ref.func) (i32.const 1234))

(assert_return (invoke "grow" (i64.const 1)) (i64.const 10) (i32.const 1234))
(assert_return (invoke "grow" (i64.const 2)) (i64.const -1) (i32.const 1234))
(assert_return (invoke "grow" (i64.const 0x100000000)) (i64.const -1) (i32.const 1234))

(assert_return (invoke "fill" (i64.const 9) (i64.const 1)) (i32.const 1234))
(assert_trap (invoke "fill" (i64.const 0x100000000) (i64.const 1)) "out of bounds table access")
(assert_trap (invoke "fill" (i64.const 0) (i64.const 0x100000001)) "out of bounds table access")

(assert_return (invoke "init" (i64.const 10) (i32.const 2) (i32.const 1)) (i32.const 1234))
(assert_trap (invoke "init" (i64.const 0x100000000) (i32.const 2) (i32.const 1)) "out of bounds table access")

(assert_return (invoke "copy" (i64.const 10) (i64.const 0) (i64.const 1)) (i32.const 1234))
(assert_trap (invoke "copy" (i64.const 0x10000000a) (i64.const 0) (i64.const 1)) "out of bounds table access")
(assert_trap (invoke "copy" (i64.const 10) (i64.const 0x100000000) (i64.const 1)) "out of bounds table access")
(assert_trap (invoke "copy" (i64.const 10) (i64.const 0) (i64.const 0x10000001)) "out of bounds table access")
