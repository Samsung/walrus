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
