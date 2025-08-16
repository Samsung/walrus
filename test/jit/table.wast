;; Basic testing

(module
  (table $t 10 externref)

  (func (export "fill") (param $i i32) (param $r externref) (param $n i32)
    (table.fill $t (local.get $i) (local.get $r) (local.get $n))
  )

  (func (export "get") (param $i i32) (result externref)
    (table.get $t (local.get $i))
  )
)

(assert_return (invoke "get" (i32.const 1)) (ref.null extern))
(assert_return (invoke "get" (i32.const 2)) (ref.null extern))
(assert_return (invoke "get" (i32.const 3)) (ref.null extern))
(assert_return (invoke "get" (i32.const 4)) (ref.null extern))
(assert_return (invoke "get" (i32.const 5)) (ref.null extern))

(assert_return (invoke "fill" (i32.const 2) (ref.extern 1) (i32.const 3)))
(assert_return (invoke "get" (i32.const 1)) (ref.null extern))
(assert_return (invoke "get" (i32.const 2)) (ref.extern 1))
(assert_return (invoke "get" (i32.const 3)) (ref.extern 1))
(assert_return (invoke "get" (i32.const 4)) (ref.extern 1))
(assert_return (invoke "get" (i32.const 5)) (ref.null extern))

(assert_return (invoke "fill" (i32.const 4) (ref.extern 2) (i32.const 2)))
(assert_return (invoke "get" (i32.const 3)) (ref.extern 1))
(assert_return (invoke "get" (i32.const 4)) (ref.extern 2))
(assert_return (invoke "get" (i32.const 5)) (ref.extern 2))
(assert_return (invoke "get" (i32.const 6)) (ref.null extern))

(assert_return (invoke "fill" (i32.const 4) (ref.extern 3) (i32.const 0)))
(assert_return (invoke "get" (i32.const 3)) (ref.extern 1))
(assert_return (invoke "get" (i32.const 4)) (ref.extern 2))
(assert_return (invoke "get" (i32.const 5)) (ref.extern 2))

(assert_return (invoke "fill" (i32.const 8) (ref.extern 4) (i32.const 2)))
(assert_return (invoke "get" (i32.const 7)) (ref.null extern))
(assert_return (invoke "get" (i32.const 8)) (ref.extern 4))
(assert_return (invoke "get" (i32.const 9)) (ref.extern 4))

(assert_return (invoke "fill" (i32.const 9) (ref.null extern) (i32.const 1)))
(assert_return (invoke "get" (i32.const 8)) (ref.extern 4))
(assert_return (invoke "get" (i32.const 9)) (ref.null extern))

(assert_return (invoke "fill" (i32.const 10) (ref.extern 5) (i32.const 0)))
(assert_return (invoke "get" (i32.const 9)) (ref.null extern))

(assert_trap
  (invoke "fill" (i32.const 8) (ref.extern 6) (i32.const 3))
  "out of bounds table access"
)
(assert_return (invoke "get" (i32.const 7)) (ref.null extern))
(assert_return (invoke "get" (i32.const 8)) (ref.extern 4))
(assert_return (invoke "get" (i32.const 9)) (ref.null extern))

(assert_trap
  (invoke "fill" (i32.const 11) (ref.null extern) (i32.const 0))
  "out of bounds table access"
)

(assert_trap
  (invoke "fill" (i32.const 11) (ref.null extern) (i32.const 10))
  "out of bounds table access"
)

;; Register shuffling

(module
  (table $table 4 6 funcref)

  (type $t (func (result i32)))

  (func $f1 (type $t) i32.const 111)
  (func $f2 (type $t) i32.const 222)
  (func $f3 (type $t) i32.const 333)
  (func $f4 (type $t) i32.const 444)

  (elem (table $table) (i32.const 0) func $f1 $f2 $f3 $f4)

  (func $grow (param i32 funcref)
    local.get 1
    local.get 0
    table.grow $table
    drop
  )

  (func (export "grow_test") (result i32)
    i32.const 2
    ref.func $f1
    call $grow
    i32.const 4
    call_indirect $table (type $t)
  )

  (func $fill_1 (param funcref i32 i32)
    local.get 2
    local.get 0
    local.get 1
    table.fill $table
  )

  (func (export "fill_test1") (result i32)
    ref.func $f2
    i32.const 2
    i32.const 4
    call $fill_1
    i32.const 4
    call_indirect $table (type $t)
  )

  (func $fill_2 (param i32 i32 funcref)
    local.get 1
    local.get 2
    local.get 0
    table.fill $table
  )

  (func (export "fill_test2") (result i32)
    i32.const 4
    i32.const 2
    ref.func $f3
    call $fill_2
    i32.const 4
    call_indirect $table (type $t)
  )

  (func $fill_3 (param funcref i32 i32)
    local.get 1
    local.get 0
    local.get 2
    table.fill $table
  )

  (func (export "fill_test3") (result i32)
    ref.func $f4
    i32.const 4
    i32.const 2
    call $fill_3
    i32.const 4
    call_indirect $table (type $t)
  )

  (func $fill_4 (param funcref i32)
    local.get 1
    local.get 0
    local.get 1
    table.fill $table
  )

  (func (export "fill_test4") (result i32)
    ref.func $f1
    i32.const 3
    call $fill_4
    i32.const 4
    call_indirect $table (type $t)
  )
)

(assert_return (invoke "grow_test") (i32.const 111))
(assert_return (invoke "fill_test1") (i32.const 222))
(assert_return (invoke "fill_test2") (i32.const 333))
(assert_return (invoke "fill_test3") (i32.const 444))
(assert_return (invoke "fill_test4") (i32.const 111))
