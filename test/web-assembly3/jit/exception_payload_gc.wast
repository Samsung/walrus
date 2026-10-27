;; Samsung/walrus#551: an exnref must keep reference payloads alive.
(module
  (import "spectest" "walrus_gc" (func $gc))
  (type $s (struct (field i32)))
  (type $a (array (mut i64)))
  (tag $t (param (ref $s)))
  (global $e (mut exnref) (ref.null exn))
  (func $make (result (ref $s)) (struct.new $s (i32.const 42)))
  (func (export "f") (result i32)
    (local $i i32) (local $junk anyref)
    (block $h (result exnref)
      (try_table (catch_all_ref $h)
        (throw $t (call $make)))
      (unreachable))
    global.set $e
    ;; Only the exception reaches the struct while allocations trigger GC.
    (loop $l
      (local.set $junk (array.new_default $a (i32.const 4096)))
      (local.set $junk (struct.new $s (i32.const 7)))
      (local.set $i (i32.add (local.get $i) (i32.const 1)))
      (br_if $l (i32.lt_u (local.get $i) (i32.const 300000))))
    (block $c (result (ref $s))
      (try_table (catch $t $c)
        (throw_ref (global.get $e)))
      (unreachable))
    (struct.get $s 0))
  (func (export "clear")
    (global.set $e (ref.null exn)))
  (func (export "gc")
    (call $gc)))

(assert_return (invoke "f") (i32.const 42))

;; Release the exception and collect its payload before store teardown.
(assert_return (invoke "clear"))
(assert_return (invoke "gc"))
(assert_return (invoke "gc"))
