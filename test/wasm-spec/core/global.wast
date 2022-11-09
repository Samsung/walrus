;; Test globals

(module

  (global (import "spectest" "global_i32") i32)
  (global (import "spectest" "global_i64") i64)

  (global $a i32 (i32.const -2))
  (global f32 (f32.const -3))
  (global f64 (f64.const -4))
  (global $b i64 (i64.const -5))

  (global $x (mut i32) (i32.const -12))
  (global (mut f32) (f32.const -13))
  (global (mut f64) (f64.const -14))
  (global $y (mut i64) (i64.const -15))

  (global $z1 i32 (global.get 0))
  (global $z2 i64 (global.get 1))

  (func (export "get-a") (result i32) (global.get $a))
  (func (export "get-b") (result i64) (global.get $b))
  (func (export "get-x") (result i32) (global.get $x))
  (func (export "get-y") (result i64) (global.get $y))
  (func (export "get-z1") (result i32) (global.get $z1))
  (func (export "get-z2") (result i64) (global.get $z2))
  (func (export "set-x") (param i32) (global.set $x (local.get 0)))
  (func (export "set-y") (param i64) (global.set $y (local.get 0)))

  (func (export "get-3") (result f32) (global.get 3))
  (func (export "get-4") (result f64) (global.get 4))
  (func (export "get-7") (result f32) (global.get 7))
  (func (export "get-8") (result f64) (global.get 8))
  (func (export "set-7") (param f32) (global.set 7 (local.get 0)))
  (func (export "set-8") (param f64) (global.set 8 (local.get 0)))

  ;; As the argument of control constructs and instructions

  (memory 1)

  (func $dummy)

  (func (export "as-select-first") (result i32)
    (select (global.get $x) (i32.const 2) (i32.const 3))
  )
  (func (export "as-select-mid") (result i32)
    (select (i32.const 2) (global.get $x) (i32.const 3))
  )
  (func (export "as-select-last") (result i32)
    (select (i32.const 2) (i32.const 3) (global.get $x))
  )

  (func (export "as-loop-first") (result i32)
    (loop (result i32)
      (global.get $x) (call $dummy) (call $dummy)
    )
  )
  (func (export "as-loop-mid") (result i32)
    (loop (result i32)
      (call $dummy) (global.get $x) (call $dummy)
    )
  )
  (func (export "as-loop-last") (result i32)
    (loop (result i32)
      (call $dummy) (call $dummy) (global.get $x)
    )
  )

  (func (export "as-if-condition") (result i32)
    (if (result i32) (global.get $x)
      (then (call $dummy) (i32.const 2))
      (else (call $dummy) (i32.const 3))
    )
  )
  (func (export "as-if-then") (result i32)
    (if (result i32) (i32.const 1)
      (then (global.get $x)) (else (i32.const 2))
    )
  )
  (func (export "as-if-else") (result i32)
    (if (result i32) (i32.const 0)
      (then (i32.const 2)) (else (global.get $x))
    )
  )

  (func (export "as-br_if-first") (result i32)
    (block (result i32)
      (br_if 0 (global.get $x) (i32.const 2))
      (return (i32.const 3))
    )
  )

  (func (export "as-br_if-last") (result i32)
    (block (result i32)
      (br_if 0 (i32.const 2) (global.get $x))
      (return (i32.const 3))
    )
  )

  (func (export "as-br_table-first") (result i32)
    (block (result i32)
      (global.get $x) (i32.const 2) (br_table 0 0)
    )
  )
  (func (export "as-br_table-last") (result i32)
    (block (result i32)
      (i32.const 2) (global.get $x) (br_table 0 0)
    )
  )
)


(assert_return (invoke "get-a") (i32.const -2))
(assert_return (invoke "get-b") (i64.const -5))
(assert_return (invoke "get-x") (i32.const -12))
(assert_return (invoke "get-y") (i64.const -15))
(assert_return (invoke "get-z1") (i32.const 666))
(assert_return (invoke "get-z2") (i64.const 666))

(assert_return (invoke "get-3") (f32.const -3))
(assert_return (invoke "get-4") (f64.const -4))
(assert_return (invoke "get-7") (f32.const -13))
(assert_return (invoke "get-8") (f64.const -14))

(assert_return (invoke "set-x" (i32.const 6)))
(assert_return (invoke "set-y" (i64.const 7)))

(assert_return (invoke "set-7" (f32.const 8)))
(assert_return (invoke "set-8" (f64.const 9)))

(assert_return (invoke "get-x") (i32.const 6))
(assert_return (invoke "get-y") (i64.const 7))
(assert_return (invoke "get-7") (f32.const 8))
(assert_return (invoke "get-8") (f64.const 9))

(assert_return (invoke "set-7" (f32.const 8)))
(assert_return (invoke "set-8" (f64.const 9)))

(assert_return (invoke "get-x") (i32.const 6))
(assert_return (invoke "get-y") (i64.const 7))
(assert_return (invoke "get-7") (f32.const 8))
(assert_return (invoke "get-8") (f64.const 9))

(assert_return (invoke "as-select-first") (i32.const 6))
(assert_return (invoke "as-select-mid") (i32.const 2))
(assert_return (invoke "as-select-last") (i32.const 2))

(assert_return (invoke "as-loop-first") (i32.const 6))
(assert_return (invoke "as-loop-mid") (i32.const 6))
(assert_return (invoke "as-loop-last") (i32.const 6))

(assert_return (invoke "as-if-condition") (i32.const 2))
(assert_return (invoke "as-if-then") (i32.const 6))
(assert_return (invoke "as-if-else") (i32.const 6))

(assert_return (invoke "as-br_if-first") (i32.const 6))
(assert_return (invoke "as-br_if-last") (i32.const 2))

(assert_return (invoke "as-br_table-first") (i32.const 6))
(assert_return (invoke "as-br_table-last") (i32.const 2))

