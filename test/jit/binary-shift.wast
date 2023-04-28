(module
(func (export "test1") (result i64 i64 i64 i64) (local i64)
  i64.const 0xff000000ff
  local.tee 0
  i64.const 0xfffe1
  i64.shl

  local.get 0
  i64.const 0xfffe1
  i64.shr_u

  local.get 0
  i64.const 0xfffe1
  i64.shr_s

  i64.const 0x8000000000000000
  i64.const 0xfffe1
  i64.shr_s

  (; 2190433320960, 127, 127, 18446744072635809792 ;)
)

(func (export "test2") (result i64 i64 i64 i64) (local i64)
  i64.const 0xff0000ffff0000ff
  local.tee 0
  i64.const 0xfffc3
  i64.shl

  local.get 0
  i64.const 0xfffc3
  i64.shr_u

  local.get 0
  i64.const 0xfffc3
  i64.shr_s

  i64.const 0x8000000900000000
  i64.const 0xfffc3
  i64.shr_s

  (; 17870292117364934648, 2296835947395809311, 18437737011891666975, 17293822573934542848 ;)
)

(func (export "test3") (result i64 i64 i64 i64) (local i64 i64)
  i64.const 0xfffe1
  local.set 0

  i64.const 0xff000000ff
  local.tee 1
  local.get 0
  i64.shl

  local.get 1
  local.get 0
  i64.shr_u

  local.get 1
  local.get 0
  i64.shr_s

  i64.const 0x8000000000000000
  local.get 0
  i64.shr_s

  (; 2190433320960, 127, 127, 18446744072635809792 ;)
)

(func (export "test4") (result i64 i64 i64 i64) (local i64 i64)
  i64.const 0xfffc3
  local.set 0

  i64.const 0xff0000ffff0000ff
  local.tee 1
  local.get 0
  i64.shl

  local.get 1
  local.get 0
  i64.shr_u

  local.get 1
  local.get 0
  i64.shr_s

  i64.const 0x8000000900000000
  local.get 0
  i64.shr_s

  (; 17870292117364934648, 2296835947395809311, 18437737011891666975, 17293822573934542848 ;)
)

(func (export "test5") (result i64 i64 i64 i64) (local i64 i64)
  i64.const 0xed00fedcba009876
  local.set 0

  i64.const 0xedcba00987edc00f
  local.set 1

  i64.const 0xedcb000edcba001f
  i64.const 0xffc8
  i64.rotl

  local.get 0
  i64.const 0xffc8
  i64.rotr

  local.get 1
  i64.const 0xffe8
  i64.rotl

  i64.const 0xed0098edcba00cba
  i64.const 0xffe8
  i64.rotr

  (; 14627707930875535341, 8569506760580792472, 17131710496515295623, 17134965183797330072 ;)
)

(func (export "test6") (result i64 i64 i64 i64) (local i64 i64 i64 i64)
  i64.const 0xffc8
  local.set 0

  i64.const 0xffe8
  local.set 1

  i64.const 0xed00fedcba009876
  local.set 2

  i64.const 0xedcba00987edc00f
  local.set 3

  i64.const 0xedcb000edcba001f
  local.get 0
  i64.rotl

  local.get 2
  local.get 0
  i64.rotr

  local.get 3
  local.get 1
  i64.rotl

  i64.const 0xed0098edcba00cba
  local.get 1
  i64.rotr

  (; 14627707930875535341, 8569506760580792472, 17131710496515295623, 17134965183797330072 ;)
)
)

(assert_return (invoke "test1") (i64.const 2190433320960) (i64.const 127) (i64.const 127) (i64.const 18446744072635809792))
(assert_return (invoke "test2") (i64.const 17870292117364934648) (i64.const 2296835947395809311) (i64.const 18437737011891666975) (i64.const 17293822573934542848))
(assert_return (invoke "test3") (i64.const 2190433320960) (i64.const 127) (i64.const 127) (i64.const 18446744072635809792))
(assert_return (invoke "test4") (i64.const 17870292117364934648) (i64.const 2296835947395809311) (i64.const 18437737011891666975) (i64.const 17293822573934542848))
(assert_return (invoke "test5") (i64.const 14627707930875535341) (i64.const 8569506760580792472) (i64.const 17131710496515295623) (i64.const 17134965183797330072))
(assert_return (invoke "test6") (i64.const 14627707930875535341) (i64.const 8569506760580792472) (i64.const 17131710496515295623) (i64.const 17134965183797330072))
