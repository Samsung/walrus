;; wait/notify with non-zero offsets
(module
  (memory 1 1 shared)

  (func (export "initOffset") (param $value i64) (param $offset i32) (i64.store offset=0 (local.get $offset) (local.get $value)))

  (func (export "memory.atomic.notify") (param $addr i32) (param $count i32) (result i32)
      (memory.atomic.notify offset=245 (local.get 0) (local.get 1)))
  (func (export "memory.atomic.wait32") (param $addr i32) (param $expected i32) (param $timeout i64) (result i32)
      (memory.atomic.wait32 offset=57822 (local.get 0) (local.get 1) (local.get 2)))
  (func (export "memory.atomic.wait64") (param $addr i32) (param $expected i64) (param $timeout i64) (result i32)
      (memory.atomic.wait64 offset=32456 (local.get 0) (local.get 1) (local.get 2)))
)

;; non-zero offsets

(invoke "initOffset" (i64.const 0xffffffffffff) (i32.const 368))
(assert_return (invoke "memory.atomic.notify" (i32.const 123) (i32.const 10)) (i32.const 0))

(invoke "initOffset" (i64.const 0xffffffffffff) (i32.const 57944))
(assert_return (invoke "memory.atomic.wait32" (i32.const 122) (i32.const 0) (i64.const 0)) (i32.const 1))

(invoke "initOffset" (i64.const 0xffffffffffff) (i32.const 32584))
(assert_return (invoke "memory.atomic.wait64" (i32.const 128) (i64.const 0xffffffffffff) (i64.const 10)) (i32.const 2))
