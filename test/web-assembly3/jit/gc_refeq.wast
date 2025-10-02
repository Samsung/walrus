(module
  (type $vec (array i8))

  (elem $e (ref $vec)
    (array.new $vec (i32.const 7) (i32.const 3))
  )

  (table $t 3 (ref null $vec))

  (func (export "test") (result i32 i32)
    (table.init $t $e (i32.const 0) (i32.const 0) (i32.const 1))
    (table.init $t $e (i32.const 1) (i32.const 0) (i32.const 1))
    (table.set (i32.const 2) (array.new $vec (i32.const 7) (i32.const 3)))

    (ref.eq (table.get $t (i32.const 0)) (table.get $t (i32.const 1)))
    (ref.eq (table.get $t (i32.const 0)) (table.get $t (i32.const 2)))
  )
)

(assert_return (invoke "test") (i32.const 1) (i32.const 0))
