;; A null must stay nullable after a nullable cast, a conversion,
;; or a typed select.

(module
  (type $s (struct (field i32)))
  (type $a (array i32))

  ;; 0: null, 1: a struct, other values: an i31 reference
  (func $any (param i32) (result anyref)
    (if (result anyref) (i32.eqz (local.get 0))
      (then (ref.null any))
      (else
        (if (result anyref) (i32.eq (local.get 0) (i32.const 1))
          (then (struct.new $s (i32.const 7)))
          (else (ref.i31 (local.get 0)))))))

  ;; 0: null, other values: an array
  (func $array (param i32) (result arrayref)
    (if (result arrayref) (local.get 0)
      (then (array.new $a (i32.const 8) (i32.const 1)))
      (else (ref.null array))))

  (func $f)
  (elem declare func $f)

  (func (export "cast_struct_get") (param i32) (result i32)
    (struct.get $s 0 (ref.cast (ref null $s) (call $any (local.get 0)))))

  (func (export "cast_array_get") (param i32) (result i32)
    (array.get $a (ref.cast (ref null $a) (call $array (local.get 0))) (i32.const 0)))

  (func (export "cast_array_len") (param i32) (result i32)
    (array.len (ref.cast arrayref (call $array (local.get 0)))))

  (func (export "cast_i31_get") (param i32) (result i32)
    (i31.get_s (ref.cast i31ref (call $any (local.get 0)))))

  (func (export "cast_cast") (param i32) (result i32)
    (ref.is_null (ref.cast arrayref (ref.cast arrayref (call $array (local.get 0))))))

  (func (export "cast_test") (param i32) (result i32)
    (ref.test (ref null $s) (ref.cast structref (call $any (local.get 0)))))

  (func (export "cast_cast_func") (param i32) (result i32)
    (ref.is_null
      (ref.cast funcref
        (ref.cast funcref
          (if (result funcref) (local.get 0)
            (then (ref.func $f))
            (else (ref.null func)))))))

  (func (export "br_on_cast_fail_struct_get") (param i32) (result i32)
    (drop
      (block $b (result anyref)
        (return (struct.get $s 0 (br_on_cast_fail $b anyref (ref null $s) (call $any (local.get 0)))))))
    (i32.const -1))

  (func (export "br_on_cast_fail_array_len") (param i32) (result i32)
    (drop
      (block $b (result anyref)
        (return (array.len (br_on_cast_fail $b anyref arrayref (call $array (local.get 0)))))))
    (i32.const -1))

  (func (export "extern_convert_test") (param i32) (result i32)
    (ref.test externref (extern.convert_any (call $any (local.get 0)))))

  (func (export "any_convert_struct_get") (param i32) (result i32)
    (struct.get $s 0
      (ref.cast (ref null $s) (any.convert_extern (extern.convert_any (call $any (local.get 0)))))))

  ;; The second operand is not nullable.
  (func (export "select_i31_get") (param i32) (result i32)
    (i31.get_s (select (result i31ref) (ref.null i31) (ref.i31 (i32.const 5)) (local.get 0))))

  (func (export "select_array_len") (param i32) (result i32)
    (array.len
      (select (result arrayref) (ref.null array) (array.new $a (i32.const 8) (i32.const 1)) (local.get 0))))

  (func (export "select_test") (param i32) (result i32)
    (ref.test anyref (select (result anyref) (ref.null any) (ref.i31 (i32.const 5)) (local.get 0))))
)

(assert_trap (invoke "cast_struct_get" (i32.const 0)) "null structure reference")
(assert_return (invoke "cast_struct_get" (i32.const 1)) (i32.const 7))
(assert_trap (invoke "cast_struct_get" (i32.const 2)) "cast failure")

(assert_trap (invoke "cast_array_get" (i32.const 0)) "null array reference")
(assert_return (invoke "cast_array_get" (i32.const 1)) (i32.const 8))

(assert_trap (invoke "cast_array_len" (i32.const 0)) "null array reference")
(assert_return (invoke "cast_array_len" (i32.const 1)) (i32.const 1))

(assert_trap (invoke "cast_i31_get" (i32.const 0)) "null i31 reference")
(assert_return (invoke "cast_i31_get" (i32.const 9)) (i32.const 9))

(assert_return (invoke "cast_cast" (i32.const 0)) (i32.const 1))
(assert_return (invoke "cast_cast" (i32.const 1)) (i32.const 0))

(assert_return (invoke "cast_test" (i32.const 0)) (i32.const 1))
(assert_return (invoke "cast_test" (i32.const 1)) (i32.const 1))

(assert_return (invoke "cast_cast_func" (i32.const 0)) (i32.const 1))
(assert_return (invoke "cast_cast_func" (i32.const 1)) (i32.const 0))

(assert_trap (invoke "br_on_cast_fail_struct_get" (i32.const 0)) "null structure reference")
(assert_return (invoke "br_on_cast_fail_struct_get" (i32.const 1)) (i32.const 7))
(assert_return (invoke "br_on_cast_fail_struct_get" (i32.const 2)) (i32.const -1))

(assert_trap (invoke "br_on_cast_fail_array_len" (i32.const 0)) "null array reference")
(assert_return (invoke "br_on_cast_fail_array_len" (i32.const 1)) (i32.const 1))

(assert_return (invoke "extern_convert_test" (i32.const 0)) (i32.const 1))
(assert_return (invoke "extern_convert_test" (i32.const 1)) (i32.const 1))

(assert_trap (invoke "any_convert_struct_get" (i32.const 0)) "null structure reference")
(assert_return (invoke "any_convert_struct_get" (i32.const 1)) (i32.const 7))

(assert_trap (invoke "select_i31_get" (i32.const 1)) "null i31 reference")
(assert_return (invoke "select_i31_get" (i32.const 0)) (i32.const 5))

(assert_trap (invoke "select_array_len" (i32.const 1)) "null array reference")
(assert_return (invoke "select_array_len" (i32.const 0)) (i32.const 1))

(assert_return (invoke "select_test" (i32.const 1)) (i32.const 1))
(assert_return (invoke "select_test" (i32.const 0)) (i32.const 1))
