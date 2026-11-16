;; array.len must trap on null references, including values loaded by array.get.
(module
  (type $a (array (mut i32)))
  (type $arrays (array (mut arrayref)))
  (global $items (ref $arrays) (array.new_default $arrays (i32.const 2)))

  (func (export "from_element") (param i32) (result i32)
    (array.len (array.get $arrays (global.get $items) (local.get 0))))
  (func (export "set_element") (param i32 i32)
    (array.set $arrays (global.get $items) (local.get 0)
      (array.new_default $a (local.get 1))))

  (func (export "null_literal") (result i32)
    (array.len (ref.null array)))
  (func $len (param arrayref) (result i32)
    (array.len (local.get 0)))
  (func (export "null_parameter") (result i32)
    (call $len (ref.null array)))
  (func (export "nullable_parameter") (param i32) (result i32)
    (call $len (array.new_default $a (local.get 0))))
  (func (export "non_null") (param i32) (result i32)
    (array.len (array.new_default $a (local.get 0))))
)

(assert_trap (invoke "from_element" (i32.const 0)) "null array reference")
(assert_trap (invoke "null_literal") "null array reference")
(assert_trap (invoke "null_parameter") "null array reference")

;; Nullable references can also point to empty or nonempty arrays.
(assert_return (invoke "set_element" (i32.const 0) (i32.const 0)))
(assert_return (invoke "from_element" (i32.const 0)) (i32.const 0))
(assert_return (invoke "set_element" (i32.const 0) (i32.const 17)))
(assert_return (invoke "from_element" (i32.const 0)) (i32.const 17))
(assert_trap (invoke "from_element" (i32.const 1)) "null array reference")
(assert_return (invoke "nullable_parameter" (i32.const 0)) (i32.const 0))
(assert_return (invoke "nullable_parameter" (i32.const 19)) (i32.const 19))
(assert_return (invoke "non_null" (i32.const 0)) (i32.const 0))
(assert_return (invoke "non_null" (i32.const 23)) (i32.const 23))
