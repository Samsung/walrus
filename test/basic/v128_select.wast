(module
  (func (export "select128") (param i32) (result v128)
    v128.const i64x2 0xaaaaaaaaaaaaaaaa 0xaaaaaaaaaaaaaaaa
    v128.const i64x2 0xbbbbbbbbbbbbbbbb 0xbbbbbbbbbbbbbbbb
    local.get 0
    select
  )

  (func (export "cmp_select128") (param i32) (result v128)
    v128.const i64x2 0xaaaaaaaaaaaaaaaa 0xaaaaaaaaaaaaaaaa
    v128.const i64x2 0xbbbbbbbbbbbbbbbb 0xbbbbbbbbbbbbbbbb
    local.get 0
    i32.const 1234
    i32.eq
    select
  )
)

(assert_return (invoke "select128" (i32.const -1)) (v128.const i64x2 0xaaaaaaaaaaaaaaaa 0xaaaaaaaaaaaaaaaa))
(assert_return (invoke "select128" (i32.const 0)) (v128.const i64x2 0xbbbbbbbbbbbbbbbb 0xbbbbbbbbbbbbbbbb))
(assert_return (invoke "cmp_select128" (i32.const 1234)) (v128.const i64x2 0xaaaaaaaaaaaaaaaa 0xaaaaaaaaaaaaaaaa))
(assert_return (invoke "cmp_select128" (i32.const 0)) (v128.const i64x2 0xbbbbbbbbbbbbbbbb 0xbbbbbbbbbbbbbbbb))
