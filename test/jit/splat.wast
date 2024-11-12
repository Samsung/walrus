(module
  (table $t 10 externref)

  (func (export "splat_f32") (result v128)
    f32.const 1234.75
    f32x4.splat
  )

  (func (export "splat_f64") (result v128)
    f64.const -123456.75
    f64x2.splat
  )
)

(assert_return (invoke "splat_f32") (v128.const f32x4 1234.75 1234.75 1234.75 1234.75))
(assert_return (invoke "splat_f64") (v128.const f64x2 -123456.75 -123456.75))
