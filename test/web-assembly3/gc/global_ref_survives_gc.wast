(module
  (import "spectest" "walrus_gc" (func $gc))
  (type $s (struct (field (mut i32))))
  (type $a (array (mut i32)))
  (global $g (mut (ref null $s)) (ref.null $s))
  (func (export "run") (result i32) (local $i i32)
    (global.set $g (struct.new $s (i32.const 305419896)))
    (loop $l
      (drop (array.new_default $a (i32.const 65536)))
      (local.set $i (i32.add (local.get $i) (i32.const 1)))
      (br_if $l (i32.lt_u (local.get $i) (i32.const 10))))
    (call $gc)
    (struct.get $s 0 (global.get $g))))

(assert_return (invoke "run") (i32.const 305419896))
