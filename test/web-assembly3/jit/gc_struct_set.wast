(module
  (type $t (struct (field (mut i64))))

  (func (export "set") (param i64) (result i64)
    (local $s (ref $t))
    (local.set $s (struct.new_default $t))
    (struct.set $t 0 (local.get $s) (local.get 0))
    (i64.add (struct.get $t 0 (local.get $s)) (struct.get $t 0 (local.get $s)))
  )
)

(assert_return (invoke "set" (i64.const 0x100000001)) (i64.const 0x200000002))
(assert_return (invoke "set" (i64.const 0x123400001234)) (i64.const 0x246800002468))
