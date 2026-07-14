(module
  (func $fib (export "fib") (param $n i32) (param $a i64) (param $b i64) (result i64)
    (if (result i64) (i32.eqz (local.get $n))
      (then (local.get $a))
      (else (return_call $fib
        (i32.sub (local.get $n) (i32.const 1))
        (local.get $b)
        (i64.add (local.get $a) (local.get $b))))))

  (func $fac (export "fac") (param $n i64) (param $acc i64) (result i64)
    (if (result i64) (i64.eqz (local.get $n))
      (then (local.get $acc))
      (else (return_call $fac
        (i64.sub (local.get $n) (i64.const 1))
        (i64.mul (local.get $n) (local.get $acc))))))

  (func $gcd (export "gcd") (param $a i64) (param $b i64) (result i64)
    (if (result i64) (i64.eqz (local.get $b))
      (then (local.get $a))
      (else (return_call $gcd
        (local.get $b)
        (i64.rem_s (local.get $a) (local.get $b))))))

  (func $collatz (export "collatz") (param $n i64) (param $steps i64) (result i64)
    (if (result i64) (i64.eq (local.get $n) (i64.const 1))
      (then (local.get $steps))
      (else (if (result i64) (i64.eqz (i64.and (local.get $n) (i64.const 1)))
        (then (return_call $collatz
          (i64.shr_u (local.get $n) (i64.const 1))
          (i64.add (local.get $steps) (i64.const 1))))
        (else (return_call $collatz
          (i64.add (i64.mul (local.get $n) (i64.const 3)) (i64.const 1))
          (i64.add (local.get $steps) (i64.const 1))))))))

  (func $count (export "count") (param $n i64) (param $acc i64) (result i64)
    (if (result i64) (i64.eqz (local.get $n))
      (then (local.get $acc))
      (else (return_call $count
        (i64.sub (local.get $n) (i64.const 1))
        (i64.add (local.get $acc) (i64.const 1)))))))

(assert_return (invoke "fib" (i32.const 0) (i64.const 0) (i64.const 1)) (i64.const 0))
(assert_return (invoke "fib" (i32.const 1) (i64.const 0) (i64.const 1)) (i64.const 1))
(assert_return (invoke "fib" (i32.const 1000000) (i64.const 0) (i64.const 1)) (i64.const 14197223477820724411))
(assert_return (invoke "fac" (i64.const 5) (i64.const 1)) (i64.const 120))
(assert_return (invoke "fac" (i64.const 1000000) (i64.const 1)) (i64.const 0))
(assert_return (invoke "fac" (i64.const 20) (i64.const 1)) (i64.const 2432902008176640000))
(assert_return (invoke "gcd" (i64.const 48) (i64.const 18)) (i64.const 6))
(assert_return (invoke "gcd" (i64.const 1071) (i64.const 462)) (i64.const 21))
(assert_return (invoke "gcd" (i64.const 4660046610375530309) (i64.const 7540113804746346429)) (i64.const 1))
(assert_return (invoke "collatz" (i64.const 27) (i64.const 0)) (i64.const 111))
(assert_return (invoke "collatz" (i64.const 837799) (i64.const 0)) (i64.const 524))
(assert_return (invoke "count" (i64.const 100) (i64.const 0)) (i64.const 100))
(assert_return (invoke "count" (i64.const 1000000) (i64.const 0)) (i64.const 1000000))

