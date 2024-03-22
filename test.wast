(module
  (memory (data
    "\01\00\00\00\01\00\00\80\01\00\00\00\01\00\00\80"
    "\01\00\00\00\01\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
  ))

  (func (export "f32.simple_x4_sum")
    (param $i i32)
    (param $j i32)
    (param $k i32)
    (local $x0 f32) (local $x1 f32) (local $x2 f32) (local $x3 f32)
    (local $y0 f32) (local $y1 f32) (local $y2 f32) (local $y3 f32)
    (local.set $x0 (f32.load offset=0 (local.get $i)))
    (local.set $x1 (f32.load offset=4 (local.get $i)))
    (local.set $x2 (f32.load offset=8 (local.get $i)))
    (local.set $x3 (f32.load offset=12 (local.get $i)))
    (local.set $y0 (f32.load offset=0 (local.get $j)))
    (local.set $y1 (f32.load offset=4 (local.get $j)))
    (local.set $y2 (f32.load offset=8 (local.get $j)))
    (local.set $y3 (f32.load offset=12 (local.get $j)))
    (f32.store offset=0 (local.get $k) (f32.add (local.get $x0) (local.get $y0)))
    (f32.store offset=4 (local.get $k) (f32.add (local.get $x1) (local.get $y1)))
    (f32.store offset=8 (local.get $k) (f32.add (local.get $x2) (local.get $y2)))
    (f32.store offset=12 (local.get $k) (f32.add (local.get $x3) (local.get $y3)))
  )

  (func (export "f32.load")
    (param $k i32) (result f32)
    (f32.load (local.get $k))
  )
)

(assert_return (invoke "f32.simple_x4_sum" (i32.const 0) (i32.const 16) (i32.const 32)))
(assert_return (invoke "f32.load" (i32.const 32)) (f32.const 0x1p-148))
(assert_return (invoke "f32.load" (i32.const 36)) (f32.const 0x0p+0))
(assert_return (invoke "f32.load" (i32.const 40)) (f32.const 0x1p-149))
(assert_return (invoke "f32.load" (i32.const 44)) (f32.const -0x1p-149))