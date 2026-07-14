(module
  (type $unary (func (param i64) (result i32)))

  (func $even (export "even") (param i64) (result i32)
    (if (result i32) (i64.eqz (local.get 0))
      (then (i32.const 44))
      (else (return_call $odd (i64.sub (local.get 0) (i64.const 1))))))
  (func $odd (export "odd") (param i64) (result i32)
    (if (result i32) (i64.eqz (local.get 0))
      (then (i32.const 99))
      (else (return_call $even (i64.sub (local.get 0) (i64.const 1))))))

  (func $sum2 (param $x i32) (param $y i32) (result i32)
    (i32.add (local.get $x) (local.get $y)))
  (func (export "double-sum") (param $n i32) (result i32)
    (return_call $sum2 (local.get $n) (i32.mul (local.get $n) (i32.const 2))))

  (table 2 funcref)
  (elem (i32.const 0) $even $odd)
  (func (export "dispatch") (param $idx i32) (param $n i64) (result i32)
    (return_call_indirect (type $unary) (local.get $n) (local.get $idx)))

  (func (export "via-ref") (param $n i64) (param $pick i32) (result i32)
    (if (result i32) (local.get $pick)
      (then (return_call_ref $unary (local.get $n) (ref.func $odd)))
      (else (return_call_ref $unary (local.get $n) (ref.func $even))))))

;; mutual recursion
(assert_return (invoke "even" (i64.const 0)) (i32.const 44))
(assert_return (invoke "even" (i64.const 1)) (i32.const 99))
(assert_return (invoke "even" (i64.const 100)) (i32.const 44))
(assert_return (invoke "odd" (i64.const 200)) (i32.const 99))
(assert_return (invoke "odd" (i64.const 77)) (i32.const 44))

;; plain A -> B
(assert_return (invoke "double-sum" (i32.const 5)) (i32.const 15))
(assert_return (invoke "double-sum" (i32.const 21)) (i32.const 63))

;; indirect dispatch (even at slot 0, odd at slot 1)
(assert_return (invoke "dispatch" (i32.const 0) (i64.const 10)) (i32.const 44))
(assert_return (invoke "dispatch" (i32.const 1) (i64.const 5)) (i32.const 44))
(assert_return (invoke "dispatch" (i32.const 0) (i64.const 7)) (i32.const 99))

;; typed reference dispatch
(assert_return (invoke "via-ref" (i64.const 10) (i32.const 0)) (i32.const 44))
(assert_return (invoke "via-ref" (i64.const 6) (i32.const 1)) (i32.const 99))
