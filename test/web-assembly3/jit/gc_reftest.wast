(module
 (func (export "struct_null") (param i32) (result i32)
   (ref.test structref (ref.i31 (local.get 0))))

 (func (export "struct_non_null") (param i32) (result i32)
   (ref.test (ref struct) (ref.i31 (local.get 0))))

 (func (export "array_null") (param i32) (result i32)
   (ref.test arrayref (ref.i31 (local.get 0))))

 (func (export "array_non_null") (param i32) (result i32)
   (ref.test (ref array) (ref.i31 (local.get 0))))
)

(assert_return (invoke "struct_null" (i32.const 0)) (i32.const 0))
(assert_return (invoke "struct_null" (i32.const 1)) (i32.const 0))
(assert_return (invoke "struct_null" (i32.const 2)) (i32.const 0))
(assert_return (invoke "struct_null" (i32.const 3)) (i32.const 0))

(assert_return (invoke "struct_non_null" (i32.const 0)) (i32.const 0))
(assert_return (invoke "struct_non_null" (i32.const 1)) (i32.const 0))
(assert_return (invoke "struct_non_null" (i32.const 2)) (i32.const 0))
(assert_return (invoke "struct_non_null" (i32.const 3)) (i32.const 0))

(assert_return (invoke "array_null" (i32.const 0)) (i32.const 0))
(assert_return (invoke "array_null" (i32.const 1)) (i32.const 0))
(assert_return (invoke "array_null" (i32.const 2)) (i32.const 0))
(assert_return (invoke "array_null" (i32.const 3)) (i32.const 0))

(assert_return (invoke "array_non_null" (i32.const 0)) (i32.const 0))
(assert_return (invoke "array_non_null" (i32.const 1)) (i32.const 0))
(assert_return (invoke "array_non_null" (i32.const 2)) (i32.const 0))
(assert_return (invoke "array_non_null" (i32.const 3)) (i32.const 0))
