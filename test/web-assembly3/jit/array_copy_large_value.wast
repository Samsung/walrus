;; Bulk instructions

(module
  (type $arr16 (array i16))
  (type $arr16_mut (array (mut i16)))

  (global $g_arr16 (ref $arr16) (array.new $arr16 (i32.const 0xbeef) (i32.const 12)))
  (global $g_arr16_mut (mut (ref $arr16_mut)) (array.new_default $arr16_mut (i32.const 12)))

  (func (export "array_get_nth") (param $1 i32) (result i32)
    (array.get_u $arr16_mut (global.get $g_arr16_mut) (local.get $1))
  )

  (func (export "array_copy") (param $1 i32) (param $2 i32) (param $3 i32)
    (array.copy $arr16_mut $arr16 (global.get $g_arr16_mut) (local.get $1) (global.get $g_arr16) (local.get $2) (local.get $3))
  )
)

;; normal case
(assert_return (invoke "array_copy" (i32.const 0) (i32.const 0) (i32.const 2)))
(assert_return (invoke "array_get_nth" (i32.const 0)) (i32.const 0xbeef))
(assert_return (invoke "array_get_nth" (i32.const 1)) (i32.const 0xbeef))
(assert_return (invoke "array_get_nth" (i32.const 2)) (i32.const 0))

(module
  (type $arr32 (array i32))
  (type $arr32_mut (array (mut i32)))

  (global $g_arr32 (ref $arr32) (array.new $arr32 (i32.const 0xdeadbeef) (i32.const 12)))
  (global $g_arr32_mut (mut (ref $arr32_mut)) (array.new_default $arr32_mut (i32.const 12)))

  (func (export "array_get_nth") (param $1 i32) (result i32)
    (array.get $arr32_mut (global.get $g_arr32_mut) (local.get $1))
  )

  (func (export "array_copy") (param $1 i32) (param $2 i32) (param $3 i32)
    (array.copy $arr32_mut $arr32 (global.get $g_arr32_mut) (local.get $1) (global.get $g_arr32) (local.get $2) (local.get $3))
  )
)

;; normal case
(assert_return (invoke "array_copy" (i32.const 2) (i32.const 1) (i32.const 3)))
(assert_return (invoke "array_get_nth" (i32.const 0)) (i32.const 0))
(assert_return (invoke "array_get_nth" (i32.const 1)) (i32.const 0))
(assert_return (invoke "array_get_nth" (i32.const 2)) (i32.const 0xdeadbeef))
(assert_return (invoke "array_get_nth" (i32.const 3)) (i32.const 0xdeadbeef))
(assert_return (invoke "array_get_nth" (i32.const 4)) (i32.const 0xdeadbeef))
(assert_return (invoke "array_get_nth" (i32.const 5)) (i32.const 0))

(module
  (type $arr64 (array i64))
  (type $arr64_mut (array (mut i64)))

  (global $g_arr64 (ref $arr64) (array.new $arr64 (i64.const 0xdeadbeef11223344) (i32.const 12)))
  (global $g_arr64_mut (mut (ref $arr64_mut)) (array.new_default $arr64_mut (i32.const 12)))

  (func (export "array_get_nth") (param $1 i32) (result i64)
    (array.get $arr64_mut (global.get $g_arr64_mut) (local.get $1))
  )

  (func (export "array_copy") (param $1 i32) (param $2 i32) (param $3 i32)
    (array.copy $arr64_mut $arr64 (global.get $g_arr64_mut) (local.get $1) (global.get $g_arr64) (local.get $2) (local.get $3))
  )
)

;; normal case
(assert_return (invoke "array_copy" (i32.const 5) (i32.const 3) (i32.const 5)))
(assert_return (invoke "array_get_nth" (i32.const 0)) (i64.const 0))
(assert_return (invoke "array_get_nth" (i32.const 1)) (i64.const 0))
(assert_return (invoke "array_get_nth" (i32.const 2)) (i64.const 0))
(assert_return (invoke "array_get_nth" (i32.const 3)) (i64.const 0))
(assert_return (invoke "array_get_nth" (i32.const 4)) (i64.const 0))
(assert_return (invoke "array_get_nth" (i32.const 5)) (i64.const 0xdeadbeef11223344))
(assert_return (invoke "array_get_nth" (i32.const 6)) (i64.const 0xdeadbeef11223344))
(assert_return (invoke "array_get_nth" (i32.const 7)) (i64.const 0xdeadbeef11223344))
(assert_return (invoke "array_get_nth" (i32.const 8)) (i64.const 0xdeadbeef11223344))
(assert_return (invoke "array_get_nth" (i32.const 9)) (i64.const 0xdeadbeef11223344))
(assert_return (invoke "array_get_nth" (i32.const 10)) (i64.const 0))

(module
  (type $arr64 (array v128))
  (type $arr64_mut (array (mut v128)))

  (global $g_arr64 (ref $arr64) (array.new $arr64 (v128.const i64x2 0xdeadbeef11223344 0xaabbccddeeff1122) (i32.const 12)))
  (global $g_arr64_mut (mut (ref $arr64_mut)) (array.new_default $arr64_mut (i32.const 12)))

  (func (export "array_get_nth") (param $1 i32) (result v128)
    (array.get $arr64_mut (global.get $g_arr64_mut) (local.get $1))
  )

  (func (export "array_copy") (param $1 i32) (param $2 i32) (param $3 i32)
    (array.copy $arr64_mut $arr64 (global.get $g_arr64_mut) (local.get $1) (global.get $g_arr64) (local.get $2) (local.get $3))
  )
)

;; normal case
(assert_return (invoke "array_copy" (i32.const 2) (i32.const 3) (i32.const 6)))
(assert_return (invoke "array_get_nth" (i32.const 0)) (v128.const i64x2 0 0))
(assert_return (invoke "array_get_nth" (i32.const 1)) (v128.const i64x2 0 0))
(assert_return (invoke "array_get_nth" (i32.const 2)) (v128.const i64x2 0xdeadbeef11223344 0xaabbccddeeff1122))
(assert_return (invoke "array_get_nth" (i32.const 3)) (v128.const i64x2 0xdeadbeef11223344 0xaabbccddeeff1122))
(assert_return (invoke "array_get_nth" (i32.const 4)) (v128.const i64x2 0xdeadbeef11223344 0xaabbccddeeff1122))
(assert_return (invoke "array_get_nth" (i32.const 5)) (v128.const i64x2 0xdeadbeef11223344 0xaabbccddeeff1122))
(assert_return (invoke "array_get_nth" (i32.const 6)) (v128.const i64x2 0xdeadbeef11223344 0xaabbccddeeff1122))
(assert_return (invoke "array_get_nth" (i32.const 7)) (v128.const i64x2 0xdeadbeef11223344 0xaabbccddeeff1122))
(assert_return (invoke "array_get_nth" (i32.const 8)) (v128.const i64x2 0 0))
