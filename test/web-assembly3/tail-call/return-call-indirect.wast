(module
  (type $acc_t (func (param i64 i64) (result i64)))
  (type $unary (func (param i64) (result i64)))

  (table $t 4 funcref)

  ;; large frame (extra locals): forces frame realloc when tail-called
  (func $acc (type $acc_t) (param $n i64) (param $s i64) (result i64)
    (local $l0 i64) (local $l1 i64) (local $l2 i64) (local $l3 i64)
    (local $l4 i64) (local $l5 i64) (local $l6 i64) (local $l7 i64)
    (if (result i64) (i64.eqz (local.get $n))
      (then (local.get $s))
      (else
        (return_call_indirect (type $acc_t)
          (i64.sub (local.get $n) (i64.const 1))
          (i64.add (local.get $s) (local.get $n))
          (i32.const 0)))))

  (func $even (type $unary) (param $n i64) (result i64)
    (if (result i64) (i64.eqz (local.get $n)) (then (i64.const 1))
      (else (return_call_indirect (type $unary) (i64.sub (local.get $n) (i64.const 1)) (i32.const 2)))))
  (func $odd (type $unary) (param $n i64) (result i64)
    (if (result i64) (i64.eqz (local.get $n)) (then (i64.const 0))
      (else (return_call_indirect (type $unary) (i64.sub (local.get $n) (i64.const 1)) (i32.const 1)))))

  (elem (table $t) (i32.const 0) $acc $even $odd)

  (func (export "sum") (param $n i64) (result i64)
    (return_call_indirect (type $acc_t) (local.get $n) (i64.const 0) (i32.const 0)))
  (func (export "is-even") (param $n i64) (result i64)
    (return_call_indirect (type $unary) (local.get $n) (i32.const 1)))

  (func (export "oob") (param $n i64) (result i64)
    (return_call_indirect (type $acc_t) (local.get $n) (i64.const 0) (i32.const 9)))
  (func (export "mismatch") (param $n i64) (result i64)
    (return_call_indirect (type $acc_t) (local.get $n) (i64.const 0) (i32.const 1)))
)

;; frame growth + deep chain
(assert_return (invoke "sum" (i64.const 0)) (i64.const 0))
(assert_return (invoke "sum" (i64.const 10)) (i64.const 55))
(assert_return (invoke "sum" (i64.const 100)) (i64.const 5050))
(assert_return (invoke "sum" (i64.const 1000000)) (i64.const 500000500000))

;; mutual recursion through the table
(assert_return (invoke "is-even" (i64.const 0)) (i64.const 1))
(assert_return (invoke "is-even" (i64.const 1)) (i64.const 0))
(assert_return (invoke "is-even" (i64.const 1000000)) (i64.const 1))
(assert_return (invoke "is-even" (i64.const 1000001)) (i64.const 0))

;; traps
(assert_trap (invoke "oob" (i64.const 5)) "undefined element")
(assert_trap (invoke "mismatch" (i64.const 5)) "indirect call type mismatch")
