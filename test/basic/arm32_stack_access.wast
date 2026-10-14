;; An i32 parameter puts the following values at offsets that are only
;; 4-byte aligned on ARM32. Exercise both stack reads and stack writes.
(module
  (func (export "i64") (param i32) (result i64)
    (local i64)
    local.get 0 i64.extend_i32_u local.set 1
    local.get 1 i64.const 40 i64.add)
  (func (export "f64") (param i32) (result f64)
    (local f64)
    local.get 0 f64.convert_i32_u local.set 1
    local.get 1 f64.const 40 f64.add)
  (func (export "i64x2") (param i32) (result i64)
    (local v128)
    local.get 0
    i64.extend_i32_u
    i64x2.splat
    local.set 1
    local.get 1
    v128.const i64x2 40 100
    i64x2.add
    local.set 1
    local.get 1
    i64x2.extract_lane 0)
  (func (export "f64x2") (param i32) (result f64)
    (local v128)
    local.get 0
    f64.convert_i32_u
    f64x2.splat
    local.set 1
    local.get 1
    v128.const f64x2 40 100
    f64x2.add
    local.set 1
    local.get 1
    f64x2.extract_lane 1)
  (func (export "i32") (param i32) (result i32)
    local.get 0
    i32.const 1
    i32.add)
  (func (export "f32") (param f32) (result f32)
    local.get 0
    f32.const 1
    f32.add))

(assert_return (invoke "i64" (i32.const 2)) (i64.const 42))
(assert_return (invoke "f64" (i32.const 2)) (f64.const 42))
(assert_return (invoke "i64x2" (i32.const 2)) (i64.const 42))
(assert_return (invoke "f64x2" (i32.const 2)) (f64.const 102))
(assert_return (invoke "i32" (i32.const 41)) (i32.const 42))
(assert_return (invoke "f32" (f32.const 41)) (f32.const 42))

;; Wide arguments, local copies, globals, and multiple results exercise
;; Value serialization as well as Const64 and MoveI64/MoveF64.
(module
  (global $i64 i64 (i64.const 40))
  (global $f64 f64 (f64.const 40))
  (func (export "i64_argument") (param i32 i64) (result i64)
    (local i64)
    local.get 1 local.set 2
    local.get 1 i64.const 1 i64.add local.set 1
    local.get 2 local.get 1 i64.add)
  (func (export "f64_argument") (param i32 f64) (result f64)
    (local f64)
    local.get 1 local.set 2
    local.get 1 f64.const 1 f64.add local.set 1
    local.get 2 local.get 1 f64.add)
  (func (export "i64_global") (param i32) (result i64)
    global.get $i64 local.get 0 i64.extend_i32_u i64.add)
  (func (export "f64_global") (param i32) (result f64)
    global.get $f64 local.get 0 f64.convert_i32_u f64.add)
  (func (export "results") (param i32) (result i32 i64 f64)
    local.get 0 i64.const 40 f64.const 42)
  ;; A Const32 before a Const64 can place the latter at a 4-byte boundary.
  (func (export "mixed_constants") (param f32 f64) (result i32)
    local.get 0 f32.const 100 f32.eq
    local.get 1 f64.const 100 f64.eq
    i32.and))

(assert_return (invoke "i64_argument" (i32.const 0) (i64.const 20))
  (i64.const 41))
(assert_return (invoke "f64_argument" (i32.const 0) (f64.const 20))
  (f64.const 41))
(assert_return (invoke "i64_global" (i32.const 2)) (i64.const 42))
(assert_return (invoke "f64_global" (i32.const 2)) (f64.const 42))
(assert_return (invoke "results" (i32.const 7))
  (i32.const 7) (i64.const 40) (f64.const 42))

(assert_return (invoke "mixed_constants" (f32.const 100) (f64.const 100))
  (i32.const 1))
