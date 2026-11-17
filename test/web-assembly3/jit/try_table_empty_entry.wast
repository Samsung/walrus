;; A function starting with an empty try_table must continue after it,
;; not in the code of its catch clause.

(module
  (type $t (func (param i32)))
  (tag $e (type $t))

  (func $trap (unreachable))

  (func (export "null_reference") (param i32) (result i32 (ref exn))
    (block (result i32 (ref exn))
      (block (result i32) (local.get 0) (try_table (catch_ref $e 1)))
      (ref.null exn)
      (ref.as_non_null)))

  (func (export "unreachable") (param i32) (result i32 (ref exn))
    (block (result i32 (ref exn))
      (block (result i32) (local.get 0) (try_table (catch_ref $e 1)))
      (unreachable)))

  (func (export "callee_trap") (param i32) (result i32 (ref exn))
    (block (result i32 (ref exn))
      (block (result i32) (local.get 0) (try_table (catch_ref $e 1)))
      (call $trap)
      (unreachable)))

  (func (export "double") (param i32) (result i32)
    (block (result i32 exnref)
      (block (result i32) (local.get 0) (try_table (catch_ref $e 1)))
      (local.get 0)
      (i32.add)
      (ref.null exn))
    (drop))
)

(assert_trap (invoke "null_reference" (i32.const 0)) "null reference")
(assert_trap (invoke "unreachable" (i32.const 0)) "unreachable")
(assert_trap (invoke "callee_trap" (i32.const 0)) "unreachable")
(assert_return (invoke "double" (i32.const 21)) (i32.const 42))
(assert_return (invoke "double" (i32.const -3)) (i32.const -6))
