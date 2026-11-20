;; The JIT gives the result of struct.new and array.new_fixed
;; a reference type, also when the result is not used.

(module
  (type $s0 (struct))
  (type $s3 (struct (field i32) (field i32) (field i32)))
  (type $a (array i32))

  (func (export "drop_struct0") (param i32) (result i32)
    (drop (struct.new $s0))
    (local.get 0))
  (func (export "drop_struct3") (param i32) (result i32)
    (drop (struct.new $s3 (i32.const 1) (i32.const 2) (i32.const 3)))
    (local.get 0))
  (func (export "drop_array0") (param i32) (result i32)
    (drop (array.new_fixed $a 0))
    (local.get 0))
  (func (export "drop_array3") (param i32) (result i32)
    (drop (array.new_fixed $a 3 (i32.const 1) (i32.const 2) (i32.const 3)))
    (local.get 0))
  (func (export "use_struct3") (result i32)
    (struct.get $s3 2 (struct.new $s3 (i32.const 1) (i32.const 2) (i32.const 3))))
  (func (export "use_array3") (result i32)
    (array.get $a (array.new_fixed $a 3 (i32.const 1) (i32.const 2) (i32.const 3)) (i32.const 1)))
)

(assert_return (invoke "drop_struct0" (i32.const 5)) (i32.const 5))
(assert_return (invoke "drop_struct3" (i32.const 6)) (i32.const 6))
(assert_return (invoke "drop_array0" (i32.const 7)) (i32.const 7))
(assert_return (invoke "drop_array3" (i32.const 8)) (i32.const 8))
(assert_return (invoke "use_struct3") (i32.const 3))
(assert_return (invoke "use_array3") (i32.const 2))
