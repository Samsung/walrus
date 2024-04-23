(module

(func (export "test1") (param i64) (result i64 i64 i64)
  i64.const 10000000
  local.get 0
  i64.mul

  i64.const -10000000
  local.get 0
  i64.mul

  i64.const -10000000
  local.set 0

  local.get 0
  local.get 0
  i64.mul

  (; 100000000000000, 18446644073709551616, 100000000000000 ;)
)

(func (export "test2") (param i32) (result i32 i32 i32)
  i32.const 10000
  local.get 0
  i32.mul

  i32.const -10000
  local.get 0
  i32.mul

  i32.const -10000
  local.set 0

  local.get 0
  local.get 0
  i32.mul

  (; 100000000, 4194967296, 100000000 ;)
)

(func (export "test3") (param i64) (result i64 i64)
  i64.const 0x3bc5600000000
  local.get 0
  i64.mul

  i64.const 0x2d961e00000001
  local.get 0
  i64.mul
)

(func (export "test4") (param i64) (result i64 i64)
  i64.const 0x572c3
  local.get 0
  i64.mul

  i64.const 0x160452d48
  local.get 0
  i64.mul
)

(func (export "test5") (param i32 i64) (result i64)
  local.get 0
  i32.const 4
  i32.add

  local.get 1
  i64.const 0x1234
  i64.add
  local.set 1

  local.set 0

  local.get 1
  local.get 1
  i64.mul
)

(func (export "test6") (param i32 i64) (result i64)
  local.get 0
  i32.const 4
  i32.add

  local.get 1
  i64.const 0x4321
  i64.add
  local.set 1

  local.set 0

  local.get 1
  i64.const 0x48b71cd0253
  i64.mul
)
)

(assert_return (invoke "test1" (i64.const 10000000)) (i64.const 100000000000000) (i64.const 18446644073709551616) (i64.const 100000000000000))
(assert_return (invoke "test2" (i32.const 10000)) (i32.const 100000000) (i32.const 4194967296) (i32.const 100000000))
(assert_return (invoke "test3" (i64.const 0x34b74b74b74b74b7)) (i64.const 0x5b34997a00000000) (i64.const 0x6a1d32e6b74b74b7))
(assert_return (invoke "test4" (i64.const 0x580f80f80f80f80f)) (i64.const 0x8016016015fea16d) (i64.const 0x5635635583eb6738))
(assert_return (invoke "test5" (i32.const 0) (i64.const 0xc48a2406a2e5)) (i64.const 0x219b6a1005485c71))
(assert_return (invoke "test6" (i32.const 0) (i64.const 0xd4b913dfe24a)) (i64.const 0x5733424463a5f7b1))
