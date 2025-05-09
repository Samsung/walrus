;; atomic operations win non-zero offsets

(module
  (memory 1 1 shared)

  (func (export "initOffset") (param $value i64) (param $offset i32) (i64.store (local.get $offset) (local.get $value)))

  (func (export "i32.atomic.load") (param $addr i32) (result i32) (i32.atomic.load offset=20 (local.get $addr)))
  (func (export "i64.atomic.load") (param $addr i32) (result i64) (i64.atomic.load offset=20 (local.get $addr)))
  (func (export "i32.atomic.load8_u") (param $addr i32) (result i32) (i32.atomic.load8_u offset=20 (local.get $addr)))
  (func (export "i32.atomic.load16_u") (param $addr i32) (result i32) (i32.atomic.load16_u offset=20 (local.get $addr)))
  (func (export "i64.atomic.load8_u") (param $addr i32) (result i64) (i64.atomic.load8_u offset=20 (local.get $addr)))
  (func (export "i64.atomic.load16_u") (param $addr i32) (result i64) (i64.atomic.load16_u offset=20 (local.get $addr)))
  (func (export "i64.atomic.load32_u") (param $addr i32) (result i64) (i64.atomic.load32_u offset=20 (local.get $addr)))

  (func (export "i32.atomic.store") (param $addr i32) (param $value i32) (i32.atomic.store offset=71 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.store") (param $addr i32) (param $value i64) (i64.atomic.store offset=71 (local.get $addr) (local.get $value)))
  (func (export "i32.atomic.store8") (param $addr i32) (param $value i32) (i32.atomic.store8 offset=71 (local.get $addr) (local.get $value)))
  (func (export "i32.atomic.store16") (param $addr i32) (param $value i32) (i32.atomic.store16 offset=71 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.store8") (param $addr i32) (param $value i64) (i64.atomic.store8 offset=71 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.store16") (param $addr i32) (param $value i64) (i64.atomic.store16 offset=71 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.store32") (param $addr i32) (param $value i64) (i64.atomic.store32 offset=71 (local.get $addr) (local.get $value)))

  (func (export "i32.atomic.rmw.add") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw.add offset=32 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw.add") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw.add offset=32 (local.get $addr) (local.get $value)))
  (func (export "i32.atomic.rmw8.add_u") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw8.add_u offset=32 (local.get $addr) (local.get $value)))
  (func (export "i32.atomic.rmw16.add_u") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw16.add_u offset=32 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw8.add_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw8.add_u offset=32 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw16.add_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw16.add_u offset=32 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw32.add_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw32.add_u offset=32 (local.get $addr) (local.get $value)))

  (func (export "i32.atomic.rmw.sub") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw.sub offset=579 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw.sub") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw.sub offset=579 (local.get $addr) (local.get $value)))
  (func (export "i32.atomic.rmw8.sub_u") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw8.sub_u offset=579 (local.get $addr) (local.get $value)))
  (func (export "i32.atomic.rmw16.sub_u") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw16.sub_u offset=579 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw8.sub_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw8.sub_u offset=579 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw16.sub_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw16.sub_u offset=579 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw32.sub_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw32.sub_u offset=579 (local.get $addr) (local.get $value)))

  (func (export "i32.atomic.rmw.and") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw.and offset=1234 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw.and") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw.and offset=1234 (local.get $addr) (local.get $value)))
  (func (export "i32.atomic.rmw8.and_u") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw8.and_u offset=1234 (local.get $addr) (local.get $value)))
  (func (export "i32.atomic.rmw16.and_u") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw16.and_u offset=1234 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw8.and_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw8.and_u offset=1234 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw16.and_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw16.and_u offset=1234 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw32.and_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw32.and_u offset=1234 (local.get $addr) (local.get $value)))

  (func (export "i32.atomic.rmw.or") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw.or offset=43523 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw.or") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw.or offset=43523 (local.get $addr) (local.get $value)))
  (func (export "i32.atomic.rmw8.or_u") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw8.or_u offset=43523 (local.get $addr) (local.get $value)))
  (func (export "i32.atomic.rmw16.or_u") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw16.or_u offset=43523 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw8.or_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw8.or_u offset=43523 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw16.or_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw16.or_u offset=43523 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw32.or_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw32.or_u offset=43523 (local.get $addr) (local.get $value)))

  (func (export "i32.atomic.rmw.xor") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw.xor offset=5372 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw.xor") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw.xor offset=5372 (local.get $addr) (local.get $value)))
  (func (export "i32.atomic.rmw8.xor_u") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw8.xor_u offset=5372 (local.get $addr) (local.get $value)))
  (func (export "i32.atomic.rmw16.xor_u") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw16.xor_u offset=5372 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw8.xor_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw8.xor_u offset=5372 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw16.xor_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw16.xor_u offset=5372 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw32.xor_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw32.xor_u offset=5372 (local.get $addr) (local.get $value)))

  (func (export "i32.atomic.rmw.xchg") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw.xchg offset=63821 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw.xchg") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw.xchg offset=63821 (local.get $addr) (local.get $value)))
  (func (export "i32.atomic.rmw8.xchg_u") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw8.xchg_u offset=63821 (local.get $addr) (local.get $value)))
  (func (export "i32.atomic.rmw16.xchg_u") (param $addr i32) (param $value i32) (result i32) (i32.atomic.rmw16.xchg_u offset=63821 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw8.xchg_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw8.xchg_u offset=63821 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw16.xchg_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw16.xchg_u offset=63821 (local.get $addr) (local.get $value)))
  (func (export "i64.atomic.rmw32.xchg_u") (param $addr i32) (param $value i64) (result i64) (i64.atomic.rmw32.xchg_u offset=63821 (local.get $addr) (local.get $value)))

  (func (export "i32.atomic.rmw.cmpxchg") (param $addr i32) (param $expected i32) (param $value i32) (result i32) (i32.atomic.rmw.cmpxchg offset=2831 (local.get $addr) (local.get $expected) (local.get $value)))
  (func (export "i64.atomic.rmw.cmpxchg") (param $addr i32) (param $expected i64)  (param $value i64) (result i64) (i64.atomic.rmw.cmpxchg offset=2831 (local.get $addr) (local.get $expected) (local.get $value)))
  (func (export "i32.atomic.rmw8.cmpxchg_u") (param $addr i32) (param $expected i32)  (param $value i32) (result i32) (i32.atomic.rmw8.cmpxchg_u offset=2831 (local.get $addr) (local.get $expected) (local.get $value)))
  (func (export "i32.atomic.rmw16.cmpxchg_u") (param $addr i32) (param $expected i32)  (param $value i32) (result i32) (i32.atomic.rmw16.cmpxchg_u offset=2831 (local.get $addr) (local.get $expected) (local.get $value)))
  (func (export "i64.atomic.rmw8.cmpxchg_u") (param $addr i32) (param $expected i64)  (param $value i64) (result i64) (i64.atomic.rmw8.cmpxchg_u offset=2831 (local.get $addr) (local.get $expected) (local.get $value)))
  (func (export "i64.atomic.rmw16.cmpxchg_u") (param $addr i32) (param $expected i64)  (param $value i64) (result i64) (i64.atomic.rmw16.cmpxchg_u offset=2831 (local.get $addr) (local.get $expected) (local.get $value)))
  (func (export "i64.atomic.rmw32.cmpxchg_u") (param $addr i32) (param $expected i64)  (param $value i64) (result i64) (i64.atomic.rmw32.cmpxchg_u offset=2831 (local.get $addr) (local.get $expected) (local.get $value)))

)

;; various non-zero offsets

(invoke "initOffset" (i64.const 0x0706050403020100) (i32.const 20))
(assert_return (invoke "i32.atomic.load16_u" (i32.const 0)) (i32.const 0x0100))
(assert_return (invoke "i32.atomic.load16_u" (i32.const 6)) (i32.const 0x0706))

(invoke "initOffset" (i64.const 0x0000000000000000) (i32.const 71))
(assert_return (invoke "i64.atomic.store32" (i32.const 5) (i64.const 0xdeadbeef)))
(assert_return (invoke "i64.atomic.load" (i32.const 52)) (i64.const 0xdeadbeef00000000))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 32))
(assert_return (invoke "i64.atomic.rmw8.add_u" (i32.const 0) (i64.const 0x4242424242424242)) (i64.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 12)) (i64.const 0x1111111111111153))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 584))
(assert_return (invoke "i32.atomic.rmw8.sub_u" (i32.const 10) (i32.const 0xcdcdcdcd)) (i32.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 564)) (i64.const 0x1111441111111111))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 1296))
(assert_return (invoke "i64.atomic.rmw16.and_u" (i32.const 66) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 1276)) (i64.const 0x1111100111111111))

(invoke "initOffset" (i64.const 0xffffffffffffffff) (i32.const 1296))
(assert_return (invoke "i64.atomic.rmw32.and_u" (i32.const 66) (i64.const 0xbeefbeef)) (i64.const 0xffffffff))
(assert_return (invoke "i64.atomic.load" (i32.const 1276)) (i64.const 0xbeefbeefffffffff))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 44280))
(assert_return (invoke "i32.atomic.rmw.or" (i32.const 757) (i32.const 0x12345678)) (i32.const 0x11111111))
(assert_return (invoke "i64.atomic.load" (i32.const 44260)) (i64.const 0x1111111113355779))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 5472))
(assert_return (invoke "i64.atomic.rmw.xor" (i32.const 100) (i64.const 0x0101010102020202)) (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.load" (i32.const 5452)) (i64.const 0x1010101013131313))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 63848))
(assert_return (invoke "i64.atomic.rmw16.xchg_u" (i32.const 31) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 63828)) (i64.const 0x1111beef11111111))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 63848))
(assert_return (invoke "i64.atomic.rmw16.xchg_u" (i32.const 29) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 63828)) (i64.const 0x11111111beef1111))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 2872))
(assert_return (invoke "i32.atomic.rmw16.cmpxchg_u" (i32.const 47) (i32.const 0x11111111) (i32.const 0xcafecafe)) (i32.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 2852)) (i64.const 0x1111111111111111))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 8216))
(assert_return (invoke "i64.atomic.rmw8.cmpxchg_u" (i32.const 5389) (i64.const 0) (i64.const 0x4242424242424242)) (i64.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 8196)) (i64.const 0x1111111111111111))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 16096))
(assert_return (invoke "i64.atomic.rmw.cmpxchg" (i32.const 13265) (i64.const 0x1111111111111111) (i64.const 0x0101010102020202)) (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.load" (i32.const 16076)) (i64.const 0x0101010102020202))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 24000))
(assert_return (invoke "i64.atomic.rmw16.cmpxchg_u" (i32.const 21169) (i64.const 0x1111) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 23980)) (i64.const 0x111111111111beef))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 8216))
(assert_return (invoke "i64.atomic.rmw8.cmpxchg_u" (i32.const 5390) (i64.const 0x11) (i64.const 0x4242424242424242)) (i64.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 8196)) (i64.const 0x1111421111111111))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 24000))
(assert_return (invoke "i64.atomic.rmw16.cmpxchg_u" (i32.const 21171) (i64.const 0x1111) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 23980)) (i64.const 0x11111111beef1111))

(invoke "initOffset" (i64.const 0xffffffffffffffff) (i32.const 24000))
(assert_return (invoke "i64.atomic.rmw16.cmpxchg_u" (i32.const 21171) (i64.const 0xffff) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0xffff))
(assert_return (invoke "i64.atomic.load" (i32.const 23980)) (i64.const 0xffffffffbeefffff))

(invoke "initOffset" (i64.const 0xffffffffffffffff) (i32.const 24000))
(assert_return (invoke "i64.atomic.rmw32.cmpxchg_u" (i32.const 21169) (i64.const 0xffffffff) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0xffffffff))
(assert_return (invoke "i64.atomic.load" (i32.const 23980)) (i64.const 0xffffffffbeefbeef))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 2872))
(assert_return (invoke "i32.atomic.rmw16.cmpxchg_u" (i32.const 47) (i32.const 0x01ff1111) (i32.const 0xcafecafe)) (i32.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 2852)) (i64.const 0x1111111111111111))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 8216))
(assert_return (invoke "i64.atomic.rmw8.cmpxchg_u" (i32.const 5389) (i64.const 0x1000000000000000) (i64.const 0x4242424242424242)) (i64.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 8196)) (i64.const 0x1111111111111111))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 16096))
(assert_return (invoke "i64.atomic.rmw.cmpxchg" (i32.const 13265) (i64.const 0x1101011111111111) (i64.const 0x0101010102020202)) (i64.const 0x1111111111111111))
(assert_return (invoke "i64.atomic.load" (i32.const 16076)) (i64.const 0x1111111111111111))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 24000))
(assert_return (invoke "i64.atomic.rmw16.cmpxchg_u" (i32.const 21169) (i64.const 0x00101111) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 23980)) (i64.const 0x1111111111111111))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 24000))
(assert_return (invoke "i64.atomic.rmw16.cmpxchg_u" (i32.const 21169) (i64.const 0x0010000000001111) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 23980)) (i64.const 0x1111111111111111))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 8216))
(assert_return (invoke "i64.atomic.rmw8.cmpxchg_u" (i32.const 5390) (i64.const 0x1010000000000011) (i64.const 0x4242424242424242)) (i64.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 8196)) (i64.const 0x1111111111111111))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 24000))
(assert_return (invoke "i64.atomic.rmw16.cmpxchg_u" (i32.const 21171) (i64.const 0x1234567800001111) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 23980)) (i64.const 0x1111111111111111))

(invoke "initOffset" (i64.const 0xffffffffffffffff) (i32.const 24000))
(assert_return (invoke "i64.atomic.rmw16.cmpxchg_u" (i32.const 21171) (i64.const 0x00f000000000ffff) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0xffff))
(assert_return (invoke "i64.atomic.load" (i32.const 23980)) (i64.const 0xffffffffffffffff))

(invoke "initOffset" (i64.const 0xffffffffffffffff) (i32.const 24000))
(assert_return (invoke "i64.atomic.rmw32.cmpxchg_u" (i32.const 21169) (i64.const 0x01010000ffffffff) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0xffffffff))
(assert_return (invoke "i64.atomic.load" (i32.const 23980)) (i64.const 0xffffffffffffffff))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 24000))
(assert_return (invoke "i64.atomic.rmw16.cmpxchg_u" (i32.const 21171) (i64.const 0x1211) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x1111))
(assert_return (invoke "i64.atomic.load" (i32.const 23980)) (i64.const 0x1111111111111111))

(invoke "initOffset" (i64.const 0x1111111111111111) (i32.const 24000))
(assert_return (invoke "i64.atomic.rmw8.cmpxchg_u" (i32.const 21171) (i64.const 0x41) (i64.const 0xbeefbeefbeefbeef)) (i64.const 0x11))
(assert_return (invoke "i64.atomic.load" (i32.const 23980)) (i64.const 0x1111111111111111))

(invoke "initOffset" (i64.const 0x8877665544332211) (i32.const 8216))
(assert_return (invoke "i64.atomic.rmw8.cmpxchg_u" (i32.const 5390) (i64.const 0x66) (i64.const 0x4242424242424242)) (i64.const 0x66))
(assert_return (invoke "i64.atomic.load" (i32.const 8196)) (i64.const 0x8877425544332211))

(invoke "initOffset" (i64.const 0x8877665544332211) (i32.const 8216))
(assert_return (invoke "i64.atomic.rmw8.cmpxchg_u" (i32.const 5390) (i64.const 0x99) (i64.const 0x4242424242424242)) (i64.const 0x66))
(assert_return (invoke "i64.atomic.load" (i32.const 8196)) (i64.const 0x8877665544332211))
