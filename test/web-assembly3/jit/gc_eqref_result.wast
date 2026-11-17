;; Results of type eqref can be checked with (ref.null eq) and (ref.eq).
(module
  (type $s (struct (field i32)))
  (type $a (array i32))

  (func (export "null") (result eqref) (ref.null eq))
  (func (export "none") (result eqref) (ref.null none))
  (func (export "i31") (result eqref) (ref.i31 (i32.const 7)))
  (func (export "struct") (result eqref) (struct.new $s (i32.const 1)))
  (func (export "array") (result eqref) (array.new $a (i32.const 1) (i32.const 2)))
  (func (export "non-null") (result (ref eq)) (ref.i31 (i32.const 7)))
  (func (export "any") (result anyref) (ref.i31 (i32.const 7)))
)

(assert_return (invoke "null") (ref.null eq))
(assert_return (invoke "none") (ref.null eq))
(assert_return (invoke "i31") (ref.eq))
(assert_return (invoke "struct") (ref.eq))
(assert_return (invoke "array") (ref.eq))
(assert_return (invoke "non-null") (ref.eq))
(assert_return (invoke "any") (ref.eq))
