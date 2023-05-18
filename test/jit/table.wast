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
