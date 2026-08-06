;; Allocation size overflow on array.new / array.new_default
;;

(module
  (type $i8 (array (mut i8)))
  (type $i32 (array (mut i32)))
  (type $i64 (array (mut i64)))
  (func (export "new") (param i32)
    (drop (array.new $i8 (i32.const 65) (local.get 0))))
  (func (export "new_default_i8") (param i32)
    (drop (array.new_default $i8 (local.get 0))))
  (func (export "new_default_i32") (param i32)
    (drop (array.new_default $i32 (local.get 0))))
  (func (export "new_default_i64") (param i32)
    (drop (array.new_default $i64 (local.get 0))))
)

(assert_trap (invoke "new" (i32.const 4294967262)) "memory allocation failed")
(assert_trap (invoke "new" (i32.const 4294967295)) "memory allocation failed")
(assert_trap (invoke "new_default_i8" (i32.const 4294967262)) "memory allocation failed")
(assert_trap (invoke "new_default_i32" (i32.const 1073741823)) "memory allocation failed")
(assert_trap (invoke "new_default_i64" (i32.const 536870911)) "memory allocation failed")
