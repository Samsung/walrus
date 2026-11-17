;; i8x16.sub_sat_u saturates each byte separately.

(module
  (func (export "i8x16") (param v128 v128) (result v128)
    (i8x16.sub_sat_u (local.get 0) (local.get 1)))
  (func (export "i16x8") (param v128 v128) (result v128)
    (i16x8.sub_sat_u (local.get 0) (local.get 1)))
)

(assert_return (invoke "i8x16"
    (v128.const i8x16 2 0 5 1 0 0 200 100 255 0 1 2 3 4 5 6)
    (v128.const i8x16 0 1 1 5 0 1 100 200 1 255 0 0 0 0 0 0))
  (v128.const i8x16 2 0 4 0 0 0 100 0 254 0 1 2 3 4 5 6))
(assert_return (invoke "i16x8"
    (v128.const i8x16 2 0 5 1 0 0 200 100 255 0 1 2 3 4 5 6)
    (v128.const i8x16 0 1 1 5 0 1 100 200 1 255 0 0 0 0 0 0))
  (v128.const i8x16 0 0 0 0 0 0 0 0 0 0 1 2 3 4 5 6))
