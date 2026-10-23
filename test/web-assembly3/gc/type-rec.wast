;; Static matching of recursive types

(module
  (rec (type $f1 (func)) (type (struct (field (ref $f1)))))
  (rec (type $f2 (func)) (type (struct (field (ref $f2)))))
  (func $f (type $f2))
  (global (ref $f1) (ref.func $f))
)

(module
  (rec (type $f1 (func)) (type (struct (field (ref $f1)))))
  (rec (type $f2 (func)) (type (struct (field (ref $f2)))))
  (rec
    (type $g1 (func))
    (type (struct (field (ref $f1) (ref $f1) (ref $f2) (ref $f2) (ref $g1))))
  )
  (rec
    (type $g2 (func))
    (type (struct (field (ref $f1) (ref $f2) (ref $f1) (ref $f2) (ref $g2))))
  )
  (func $g (type $g2))
  (global (ref $g1) (ref.func $g))
)

(assert_invalid
  (module
    (rec (type $f1 (func)) (type (struct (field (ref $f1)))))
    (rec (type $f2 (func)) (type (struct (field (ref $f1)))))
    (func $f (type $f2))
    (global (ref $f1) (ref.func $f))
  )
  "type mismatch"
)

(assert_invalid
  (module
    (rec (type $f0 (func)) (type (struct (field (ref $f0)))))
    (rec (type $f1 (func)) (type (struct (field (ref $f0)))))
    (rec (type $f2 (func)) (type (struct (field (ref $f1)))))
    (func $f (type $f2))
    (global (ref $f1) (ref.func $f))
  )
  "type mismatch"
)

(assert_invalid
  (module
    (rec (type $f1 (func)) (type (struct)))
    (rec (type (struct)) (type $f2 (func)))
    (global (ref $f1) (ref.func $f))
    (func $f (type $f2))
  )
  "type mismatch"
)

(assert_invalid
  (module
    (rec (type $f1 (func)) (type (struct)))
    (rec (type $f2 (func)) (type (struct)) (type (func)))
    (global (ref $f1) (ref.func $f))
    (func $f (type $f2))
  )
  "type mismatch"
)


;; Link-time matching of recursive function types

(module $M
  (rec (type $f1 (func)) (type (struct)))
  (func (export "f") (type $f1))
)
(register "M" $M)

(module
  (rec (type $f2 (func)) (type (struct)))
  (func (import "M" "f") (type $f2))
)

(assert_unlinkable
  (module
    (rec (type (struct)) (type $f2 (func)))
    (func (import "M" "f") (type $f2))
  )
  "incompatible import type"
)

(assert_unlinkable
  (module
    (rec (type $f2 (func)))
    (func (import "M" "f") (type $f2))
  )
  "incompatible import type"
)


;; Dynamic matching of recursive function types

(module
  (rec (type $f1 (func)) (type (struct)))
  (rec (type $f2 (func)) (type (struct)))
  (table funcref (elem $f1))
  (func $f1 (type $f1))
  (func (export "run") (call_indirect (type $f2) (i32.const 0)))
)
(assert_return (invoke "run"))

(module
  (rec (type $f1 (func)) (type (struct)))
  (rec (type (struct)) (type $f2 (func)))
  (table funcref (elem $f1))
  (func $f1 (type $f1))
  (func (export "run") (call_indirect (type $f2) (i32.const 0)))
)
(assert_trap (invoke "run") "indirect call type mismatch")

(module
  (rec (type $f1 (func)) (type (struct)))
  (rec (type $f2 (func)))
  (table funcref (elem $f1))
  (func $f1 (type $f1))
  (func (export "run") (call_indirect (type $f2) (i32.const 0)))
)
(assert_trap (invoke "run") "indirect call type mismatch")


;; Dynamic matching distinguishes positions within the same recursive group,
;; even when two positions have identical function signatures.

(module
  (rec
    (type $expected (func (result i32)))
    (type $wrong-result (func (result i64)))
    (type $wrong-param (func (param funcref) (result i32)))
    (type $same-signature (func (result i32)))
  )
  (rec
    (type $equivalent (func (result i32)))
    (type (func (result i64)))
    (type (func (param funcref) (result i32)))
    (type (func (result i32)))
  )
  (func $f (type $expected) (i32.const 42))
  (func $g (type $wrong-result) (i64.const 4294967297))
  (func $h (type $wrong-param) (unreachable))
  (func $i (type $same-signature) (i32.const 43))
  (table $t32 funcref (elem $f $g $h $i))
  (table $t64 i64 4 funcref)
  (elem (table $t64) (i64.const 0) func $f $g $h $i)

  (func (export "call32") (param i32) (result i32)
    (call_indirect $t32 (type $expected) (local.get 0)))
  (func (export "call64") (param i64) (result i32)
    (call_indirect $t64 (type $expected) (local.get 0)))
  (func (export "tail32") (param i32) (result i32)
    (return_call_indirect $t32 (type $expected) (local.get 0)))
  (func (export "tail64") (param i64) (result i32)
    (return_call_indirect $t64 (type $expected) (local.get 0)))
  (func (export "equivalent32") (result i32)
    (call_indirect $t32 (type $equivalent) (i32.const 0)))
  (func (export "equivalent64") (result i32)
    (call_indirect $t64 (type $equivalent) (i64.const 0)))
)

(assert_return (invoke "call32" (i32.const 0)) (i32.const 42))
(assert_trap (invoke "call32" (i32.const 1)) "indirect call type mismatch")
(assert_trap (invoke "call32" (i32.const 2)) "indirect call type mismatch")
(assert_trap (invoke "call32" (i32.const 3)) "indirect call type mismatch")
(assert_return (invoke "call64" (i64.const 0)) (i32.const 42))
(assert_trap (invoke "call64" (i64.const 1)) "indirect call type mismatch")
(assert_trap (invoke "call64" (i64.const 2)) "indirect call type mismatch")
(assert_trap (invoke "call64" (i64.const 3)) "indirect call type mismatch")
(assert_return (invoke "tail32" (i32.const 0)) (i32.const 42))
(assert_trap (invoke "tail32" (i32.const 1)) "indirect call type mismatch")
(assert_trap (invoke "tail32" (i32.const 2)) "indirect call type mismatch")
(assert_trap (invoke "tail32" (i32.const 3)) "indirect call type mismatch")
(assert_return (invoke "tail64" (i64.const 0)) (i32.const 42))
(assert_trap (invoke "tail64" (i64.const 1)) "indirect call type mismatch")
(assert_trap (invoke "tail64" (i64.const 2)) "indirect call type mismatch")
(assert_trap (invoke "tail64" (i64.const 3)) "indirect call type mismatch")
(assert_return (invoke "equivalent32") (i32.const 42))
(assert_return (invoke "equivalent64") (i32.const 42))


;; Implicit function types never pick up non-singleton recursive types

(module
  (rec (type $s (struct)))
  (rec (type $t (func (param (ref $s)))))
  (func $f (param (ref $s)))  ;; okay, type is equivalent to $t
  (global (ref $t) (ref.func $f))
)

(assert_invalid
  (module
    (rec
      (type $s (struct))
      (type $t (func (param (ref $s))))
    )
    (func $f (param (ref $s)))  ;; type is not equivalent to $t
    (global (ref $t) (ref.func $f))
  )
  "type mismatch"
)

(assert_invalid
  (module
    (rec
      (type (struct))
      (type $t (func))
    )
    (func $f)  ;; type is not equivalent to $t
    (global (ref $t) (ref.func $f))
  )
  "type mismatch"
)
