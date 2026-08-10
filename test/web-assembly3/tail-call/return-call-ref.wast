(module
  (type $acc_t (func (param i64 i64) (result i64)))
  (type $unary (func (param i64) (result i64)))

  (elem declare func $acc $even $odd)

  ;; large frame (extra locals): forces frame realloc when tail-called
  (func $acc (type $acc_t) (param $n i64) (param $s i64) (result i64)
    (local $l0 i64) (local $l1 i64) (local $l2 i64) (local $l3 i64)
    (local $l4 i64) (local $l5 i64) (local $l6 i64) (local $l7 i64)
    (if (result i64) (i64.eqz (local.get $n))
      (then (local.get $s))
      (else
        (return_call_ref $acc_t
          (i64.sub (local.get $n) (i64.const 1))
          (i64.add (local.get $s) (local.get $n))
          (ref.func $acc)))))

  (func $even (type $unary) (param $n i64) (result i64)
    (if (result i64) (i64.eqz (local.get $n)) (then (i64.const 1))
      (else (return_call_ref $unary (i64.sub (local.get $n) (i64.const 1)) (ref.func $odd)))))
  (func $odd (type $unary) (param $n i64) (result i64)
    (if (result i64) (i64.eqz (local.get $n)) (then (i64.const 0))
      (else (return_call_ref $unary (i64.sub (local.get $n) (i64.const 1)) (ref.func $even)))))

  (func (export "sum") (param $n i64) (result i64)
    (return_call_ref $acc_t (local.get $n) (i64.const 0) (ref.func $acc)))
  (func (export "is-even") (param $n i64) (result i64)
    (return_call_ref $unary (local.get $n) (ref.func $even)))

  (func (export "pick") (param $n i64) (param $which i32) (result i64)
    (local $f (ref null $unary))
    (if (local.get $which)
      (then (local.set $f (ref.func $odd)))
      (else (local.set $f (ref.func $even))))
    (return_call_ref $unary (local.get $n) (local.get $f)))

  (func (export "null") (param $n i64) (result i64)
    (return_call_ref $acc_t (local.get $n) (i64.const 0) (ref.null $acc_t)))
)

;; frame growth + deep chain
(assert_return (invoke "sum" (i64.const 0)) (i64.const 0))
(assert_return (invoke "sum" (i64.const 10)) (i64.const 55))
(assert_return (invoke "sum" (i64.const 100)) (i64.const 5050))
(assert_return (invoke "sum" (i64.const 1000000)) (i64.const 500000500000))

;; mutual recursion through references
(assert_return (invoke "is-even" (i64.const 0)) (i64.const 1))
(assert_return (invoke "is-even" (i64.const 1)) (i64.const 0))
(assert_return (invoke "is-even" (i64.const 1000000)) (i64.const 1))
(assert_return (invoke "is-even" (i64.const 1000001)) (i64.const 0))

;; reference chosen at runtime
(assert_return (invoke "pick" (i64.const 10) (i32.const 0)) (i64.const 1))
(assert_return (invoke "pick" (i64.const 7) (i32.const 1)) (i64.const 1))
(assert_return (invoke "pick" (i64.const 10) (i32.const 1)) (i64.const 0))

;; trap
(assert_trap (invoke "null" (i64.const 5)) "null function reference")
