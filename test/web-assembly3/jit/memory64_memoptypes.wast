(module
    (memory i64 1)

    ;; i32
    (func (export "i32.store") (param i64 i32) (i32.store (local.get 0) (local.get 1)))
    (func (export "i32.load") (param i64) (result i32) (i32.load (local.get 0)))

    (func (export "i32.store_2") (param i64 i32) (i32.store offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.load_2") (param i64) (result i32) (i32.load offset=16 (local.get 0)))

    (func (export "i32.store_3") (param i64 i32) (i32.store offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.load_3") (param i64) (result i32) (i32.load offset=0x100000000 (local.get 0)))

    ;; i32_8
    (func (export "i32.store8") (param i64 i32) (i32.store8 offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.load8u") (param i64) (result i32) (i32.load8_u offset=16 (local.get 0)))
    (func (export "i32.load8s") (param i64) (result i32) (i32.load8_s offset=16 (local.get 0)))

    (func (export "i32.store8_2") (param i64 i32) (i32.store8 offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.load8u_2") (param i64) (result i32) (i32.load8_u offset=0x100000000 (local.get 0)))
    (func (export "i32.load8s_2") (param i64) (result i32) (i32.load8_s offset=0x100000000 (local.get 0)))

    ;; i32_16
    (func (export "i32.store16") (param i64 i32) (i32.store16 offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.load16u") (param i64) (result i32) (i32.load16_u offset=16 (local.get 0)))
    (func (export "i32.load16s") (param i64) (result i32) (i32.load16_s offset=16 (local.get 0)))

    (func (export "i32.store16_2") (param i64 i32) (i32.store16 offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.load16u_2") (param i64) (result i32) (i32.load16_u offset=0x100000000 (local.get 0)))
    (func (export "i32.load16s_2") (param i64) (result i32) (i32.load16_s offset=0x100000000 (local.get 0)))

    ;; i64
    (func (export "i64.store") (param i64 i64) (i64.store (local.get 0) (local.get 1)))
    (func (export "i64.load") (param i64) (result i64) (i64.load (local.get 0)))

    (func (export "i64.store_2") (param i64 i64) (i64.store offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.load_2") (param i64) (result i64) (i64.load offset=16 (local.get 0)))

    (func (export "i64.store_3") (param i64 i64) (i64.store offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.load_3") (param i64) (result i64) (i64.load offset=0x100000000 (local.get 0)))

    ;; i64_8
    (func (export "i64.store8") (param i64 i64) (i64.store8 offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.load8u") (param i64) (result i64) (i64.load8_u offset=16 (local.get 0)))
    (func (export "i64.load8s") (param i64) (result i64) (i64.load8_s offset=16 (local.get 0)))

    (func (export "i64.store8_2") (param i64 i64) (i64.store8 offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.load8u_2") (param i64) (result i64) (i64.load8_u offset=0x100000000 (local.get 0)))
    (func (export "i64.load8s_2") (param i64) (result i64) (i64.load8_s offset=0x100000000 (local.get 0)))

    ;; i64_16
    (func (export "i64.store16") (param i64 i64) (i64.store16 offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.load16u") (param i64) (result i64) (i64.load16_u offset=16 (local.get 0)))
    (func (export "i64.load16s") (param i64) (result i64) (i64.load16_s offset=16 (local.get 0)))

    (func (export "i64.store16_2") (param i64 i64) (i64.store16 offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.load16u_2") (param i64) (result i64) (i64.load16_u offset=0x100000000 (local.get 0)))
    (func (export "i64.load16s_2") (param i64) (result i64) (i64.load16_s offset=0x100000000 (local.get 0)))

    ;; i64_32
    (func (export "i64.store32") (param i64 i64) (i64.store32 offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.load32u") (param i64) (result i64) (i64.load32_u offset=16 (local.get 0)))
    (func (export "i64.load32s") (param i64) (result i64) (i64.load32_s offset=16 (local.get 0)))

    (func (export "i64.store32_2") (param i64 i64) (i64.store32 offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.load32u_2") (param i64) (result i64) (i64.load32_u offset=0x100000000 (local.get 0)))
    (func (export "i64.load32s_2") (param i64) (result i64) (i64.load32_s offset=0x100000000 (local.get 0)))

    ;; f32
    (func (export "f32.store") (param i64 f32) (f32.store offset=16 (local.get 0) (local.get 1)))
    (func (export "f32.load") (param i64) (result f32) (f32.load offset=16 (local.get 0)))

    (func (export "f32.store_2") (param i64 f32) (f32.store offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "f32.load_2") (param i64) (result f32) (f32.load offset=0x100000000 (local.get 0)))

    ;; f64
    (func (export "f64.store") (param i64 f64) (f64.store offset=16 (local.get 0) (local.get 1)))
    (func (export "f64.load") (param i64) (result f64) (f64.load offset=16 (local.get 0)))

    (func (export "f64.store_2") (param i64 f64) (f64.store offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "f64.load_2") (param i64) (result f64) (f64.load offset=0x100000000 (local.get 0)))

    ;; v128
    (func (export "v128.store") (param i64 v128) (v128.store offset=16 (local.get 0) (local.get 1)))
    (func (export "v128.load") (param i64) (result v128) (v128.load offset=16 (local.get 0)))

    (func (export "v128.store_2") (param i64 v128) (v128.store offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "v128.load_2") (param i64) (result v128) (v128.load offset=0x100000000 (local.get 0)))

    ;; v128 8x8
    (func (export "v128.load8x8s") (param i64) (result v128) (v128.load8x8_s offset=16 (local.get 0)))
    (func (export "v128.load8x8u") (param i64) (result v128) (v128.load8x8_u offset=16 (local.get 0)))

    (func (export "v128.load8x8s_2") (param i64) (result v128) (v128.load8x8_s offset=0x100000000 (local.get 0)))
    (func (export "v128.load8x8u_2") (param i64) (result v128) (v128.load8x8_u offset=0x100000000 (local.get 0)))

    ;; v128 16x4
    (func (export "v128.load16x4s") (param i64) (result v128) (v128.load16x4_s offset=16 (local.get 0)))
    (func (export "v128.load16x4u") (param i64) (result v128) (v128.load16x4_u offset=16 (local.get 0)))

    (func (export "v128.load16x4s_2") (param i64) (result v128) (v128.load16x4_s offset=0x100000000 (local.get 0)))
    (func (export "v128.load16x4u_2") (param i64) (result v128) (v128.load16x4_u offset=0x100000000 (local.get 0)))

    ;; v128 32x2
    (func (export "v128.load32x2s") (param i64) (result v128) (v128.load32x2_s offset=16 (local.get 0)))
    (func (export "v128.load32x2u") (param i64) (result v128) (v128.load32x2_u offset=16 (local.get 0)))

    (func (export "v128.load32x2s_2") (param i64) (result v128) (v128.load32x2_s offset=0x100000000 (local.get 0)))
    (func (export "v128.load32x2u_2") (param i64) (result v128) (v128.load32x2_u offset=0x100000000 (local.get 0)))

    ;; v128 splat
    (func (export "v128.load8splat") (param i64) (result v128) (v128.load8_splat offset=16 (local.get 0)))
    (func (export "v128.load16splat") (param i64) (result v128) (v128.load16_splat offset=16 (local.get 0)))
    (func (export "v128.load32splat") (param i64) (result v128) (v128.load32_splat offset=16 (local.get 0)))
    (func (export "v128.load64splat") (param i64) (result v128) (v128.load64_splat offset=16 (local.get 0)))

    (func (export "v128.load8splat_2") (param i64) (result v128) (v128.load8_splat offset=0x100000000 (local.get 0)))
    (func (export "v128.load16splat_2") (param i64) (result v128) (v128.load16_splat offset=0x100000000 (local.get 0)))
    (func (export "v128.load32splat_2") (param i64) (result v128) (v128.load32_splat offset=0x100000000 (local.get 0)))
    (func (export "v128.load64splat_2") (param i64) (result v128) (v128.load64_splat offset=0x100000000 (local.get 0)))

    ;; v128 lane 8
    (func (export "v128.store8lane") (param i64 v128) (v128.store8_lane offset=16 0 (local.get 0) (local.get 1)))
    (func (export "v128.load8lane") (param i64) (result v128) (v128.load8_lane offset=16 0 (local.get 0) (v128.const i64x2 0 0)))

    (func (export "v128.store8lane_2") (param i64 v128) (v128.store8_lane offset=0x100000000 0 (local.get 0) (local.get 1)))
    (func (export "v128.load8lane_2") (param i64) (result v128) (v128.load8_lane offset=0x100000000 0 (local.get 0) (v128.const i64x2 0 0)))

    ;; v128 lane 16
    (func (export "v128.store16lane") (param i64 v128) (v128.store16_lane offset=16 0 (local.get 0) (local.get 1)))
    (func (export "v128.load16lane") (param i64) (result v128) (v128.load16_lane offset=16 0 (local.get 0) (v128.const i64x2 0 0)))

    (func (export "v128.store16lane_2") (param i64 v128) (v128.store16_lane offset=0x100000000 0 (local.get 0) (local.get 1)))
    (func (export "v128.load16lane_2") (param i64) (result v128) (v128.load16_lane offset=0x100000000 0 (local.get 0) (v128.const i64x2 0 0)))

    ;; v128 lane 32
    (func (export "v128.store32lane") (param i64 v128) (v128.store32_lane offset=16 0 (local.get 0) (local.get 1)))
    (func (export "v128.load32lane") (param i64) (result v128) (v128.load32_lane offset=16 0 (local.get 0) (v128.const i64x2 0 0)))

    (func (export "v128.store32lane_2") (param i64 v128) (v128.store32_lane offset=0x100000000 0 (local.get 0) (local.get 1)))
    (func (export "v128.load32lane_2") (param i64) (result v128) (v128.load32_lane offset=0x100000000 0 (local.get 0) (v128.const i64x2 0 0)))

    ;; v128 lane 64
    (func (export "v128.store64lane") (param i64 v128) (v128.store64_lane offset=16 0 (local.get 0) (local.get 1)))
    (func (export "v128.load64lane") (param i64) (result v128) (v128.load64_lane offset=16 0 (local.get 0) (v128.const i64x2 0 0)))

    (func (export "v128.store64lane_2") (param i64 v128) (v128.store64_lane offset=0x100000000 0 (local.get 0) (local.get 1)))
    (func (export "v128.load64lane_2") (param i64) (result v128) (v128.load64_lane offset=0x100000000 0 (local.get 0) (v128.const i64x2 0 0)))

    ;; v128 load zero
    (func (export "v128.load32zero") (param i64) (result v128) (v128.load32_zero offset=16 (local.get 0)))
    (func (export "v128.load64zero") (param i64) (result v128) (v128.load64_zero offset=16 (local.get 0)))

    (func (export "v128.load32zero_2") (param i64) (result v128) (v128.load32_zero offset=0x100000000 (local.get 0)))
    (func (export "v128.load64zero_2") (param i64) (result v128) (v128.load64_zero offset=0x100000000 (local.get 0)))

    ;; atomic core
    (func (export "atomic.notify") (param i64) (drop (memory.atomic.notify offset=16 (local.get 0) (i32.const 0))))
    (func (export "memory.atomic.wait32") (param i64) (drop (memory.atomic.wait32 offset=16 (local.get 0) (i32.const 0) (i64.const 0))))
    (func (export "memory.atomic.wait64") (param i64) (drop (memory.atomic.wait64 offset=16 (local.get 0) (i64.const 0) (i64.const 0))))

    (func (export "atomic.notify_2") (param i64) (drop (memory.atomic.notify offset=0x100000000 (local.get 0) (i32.const 0))))
    (func (export "memory.atomic.wait32_2") (param i64) (drop (memory.atomic.wait32 offset=0x100000000 (local.get 0) (i32.const 0) (i64.const 0))))
    (func (export "memory.atomic.wait64_2") (param i64) (drop (memory.atomic.wait64 offset=0x100000000 (local.get 0) (i64.const 0) (i64.const 0))))

    ;; i32 atomic
    (func (export "i32.atomic.store") (param i64 i32) (i32.atomic.store offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.load") (param i64) (result i32) (i32.atomic.load offset=16 (local.get 0)))

    (func (export "i32.atomic.store_2") (param i64 i32) (i32.atomic.store offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.load_2") (param i64) (result i32) (i32.atomic.load offset=0x100000000 (local.get 0)))

    ;; i32 atomic 8
    (func (export "i32.atomic.store8") (param i64 i32) (i32.atomic.store8 offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.load8") (param i64) (result i32) (i32.atomic.load8_u offset=16 (local.get 0)))

    (func (export "i32.atomic.store8_2") (param i64 i32) (i32.atomic.store8 offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.load8_2") (param i64) (result i32) (i32.atomic.load8_u offset=0x100000000 (local.get 0)))

    ;; i32 atomic 16
    (func (export "i32.atomic.store16") (param i64 i32) (i32.atomic.store16 offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.load16") (param i64) (result i32) (i32.atomic.load16_u offset=16 (local.get 0)))

    (func (export "i32.atomic.store16_2") (param i64 i32) (i32.atomic.store16 offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.load16_2") (param i64) (result i32) (i32.atomic.load16_u offset=0x100000000 (local.get 0)))

    ;; i64 atomic
    (func (export "i64.atomic.store") (param i64 i64) (i64.atomic.store offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.load") (param i64) (result i64) (i64.atomic.load offset=16 (local.get 0)))

    (func (export "i64.atomic.store_2") (param i64 i64) (i64.atomic.store offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.load_2") (param i64) (result i64) (i64.atomic.load offset=0x100000000 (local.get 0)))

    ;; i64 atomic 8
    (func (export "i64.atomic.store8") (param i64 i64) (i64.atomic.store8 offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.load8") (param i64) (result i64) (i64.atomic.load8_u offset=16 (local.get 0)))

    (func (export "i64.atomic.store8_2") (param i64 i64) (i64.atomic.store8 offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.load8_2") (param i64) (result i64) (i64.atomic.load8_u offset=0x100000000 (local.get 0)))

    ;; i64 atomic 16
    (func (export "i64.atomic.store16") (param i64 i64) (i64.atomic.store16 offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.load16") (param i64) (result i64) (i64.atomic.load16_u offset=16 (local.get 0)))

    (func (export "i64.atomic.store16_2") (param i64 i64) (i64.atomic.store16 offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.load16_2") (param i64) (result i64) (i64.atomic.load16_u offset=0x100000000 (local.get 0)))

    ;; i64 atomic 32
    (func (export "i64.atomic.store32") (param i64 i64) (i64.atomic.store32 offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.load32") (param i64) (result i64) (i64.atomic.load32_u offset=16 (local.get 0)))

    (func (export "i64.atomic.store32_2") (param i64 i64) (i64.atomic.store32 offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.load32_2") (param i64) (result i64) (i64.atomic.load32_u offset=0x100000000 (local.get 0)))

    ;; i32 atomic rmw
    (func (export "i32.atomic.rmw.add") (param i64 i32) (result i32) (i32.atomic.rmw.add offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw.sub") (param i64 i32) (result i32) (i32.atomic.rmw.sub offset=16 (local.get 0) (local.get 1)))

    (func (export "i32.atomic.rmw.add_2") (param i64 i32) (result i32) (i32.atomic.rmw.add offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw.sub_2") (param i64 i32) (result i32) (i32.atomic.rmw.sub offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i32 atomic rmw 8
    (func (export "i32.atomic.rmw8.or") (param i64 i32) (result i32) (i32.atomic.rmw8.or_u offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw8.xor") (param i64 i32) (result i32) (i32.atomic.rmw8.xor_u offset=16 (local.get 0) (local.get 1)))

    (func (export "i32.atomic.rmw8.or_2") (param i64 i32) (result i32) (i32.atomic.rmw8.or_u offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw8.xor_2") (param i64 i32) (result i32) (i32.atomic.rmw8.xor_u offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i32 atomic rmw 16
    (func (export "i32.atomic.rmw16.or") (param i64 i32) (result i32) (i32.atomic.rmw16.or_u offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw16.and") (param i64 i32) (result i32) (i32.atomic.rmw16.and_u offset=16 (local.get 0) (local.get 1)))

    (func (export "i32.atomic.rmw16.or_2") (param i64 i32) (result i32) (i32.atomic.rmw16.or_u offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw16.and_2") (param i64 i32) (result i32) (i32.atomic.rmw16.and_u offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i64 atomic rmw
    (func (export "i64.atomic.rmw.add") (param i64 i64) (result i64) (i64.atomic.rmw.add offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw.sub") (param i64 i64) (result i64) (i64.atomic.rmw.sub offset=16 (local.get 0) (local.get 1)))

    (func (export "i64.atomic.rmw.add_2") (param i64 i64) (result i64) (i64.atomic.rmw.add offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw.sub_2") (param i64 i64) (result i64) (i64.atomic.rmw.sub offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i64 atomic rmw 8
    (func (export "i64.atomic.rmw8.or") (param i64 i64) (result i64) (i64.atomic.rmw8.or_u offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw8.xor") (param i64 i64) (result i64) (i64.atomic.rmw8.xor_u offset=16 (local.get 0) (local.get 1)))

    (func (export "i64.atomic.rmw8.or_2") (param i64 i64) (result i64) (i64.atomic.rmw8.or_u offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw8.xor_2") (param i64 i64) (result i64) (i64.atomic.rmw8.xor_u offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i64 atomic rmw 16
    (func (export "i64.atomic.rmw16.or") (param i64 i64) (result i64) (i64.atomic.rmw16.or_u offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw16.and") (param i64 i64) (result i64) (i64.atomic.rmw16.and_u offset=16 (local.get 0) (local.get 1)))

    (func (export "i64.atomic.rmw16.or_2") (param i64 i64) (result i64) (i64.atomic.rmw16.or_u offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw16.and_2") (param i64 i64) (result i64) (i64.atomic.rmw16.and_u offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i64 atomic rmw 32
    (func (export "i64.atomic.rmw32.add") (param i64 i64) (result i64) (i64.atomic.rmw32.add_u offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw32.and") (param i64 i64) (result i64) (i64.atomic.rmw32.and_u offset=16 (local.get 0) (local.get 1)))

    (func (export "i64.atomic.rmw32.add_2") (param i64 i64) (result i64) (i64.atomic.rmw32.add_u offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw32.and_2") (param i64 i64) (result i64) (i64.atomic.rmw32.and_u offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i32 atomic Xchg
    (func (export "i32.atomic.rmw.xchg") (param i64 i32) (result i32) (i32.atomic.rmw.xchg offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw8.xchg") (param i64 i32) (result i32) (i32.atomic.rmw8.xchg_u offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw16.xchg") (param i64 i32) (result i32) (i32.atomic.rmw16.xchg_u offset=16 (local.get 0) (local.get 1)))

    (func (export "i32.atomic.rmw.xchg_2") (param i64 i32) (result i32) (i32.atomic.rmw.xchg offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw8.xchg_2") (param i64 i32) (result i32) (i32.atomic.rmw8.xchg_u offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw16.xchg_2") (param i64 i32) (result i32) (i32.atomic.rmw16.xchg_u offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i64 atomic Xchg
    (func (export "i64.atomic.rmw.xchg") (param i64 i64) (result i64) (i64.atomic.rmw.xchg offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw8.xchg") (param i64 i64) (result i64) (i64.atomic.rmw8.xchg_u offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw16.xchg") (param i64 i64) (result i64) (i64.atomic.rmw16.xchg_u offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw32.xchg") (param i64 i64) (result i64) (i64.atomic.rmw32.xchg_u offset=16 (local.get 0) (local.get 1)))

    (func (export "i64.atomic.rmw.xchg_2") (param i64 i64) (result i64) (i64.atomic.rmw.xchg offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw8.xchg_2") (param i64 i64) (result i64) (i64.atomic.rmw8.xchg_u offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw16.xchg_2") (param i64 i64) (result i64) (i64.atomic.rmw16.xchg_u offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw32.xchg_2") (param i64 i64) (result i64) (i64.atomic.rmw32.xchg_u offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i32 atomic Cmpxchg
    (func (export "i32.atomic.rmw.cmpxchg") (param i64 i32 i32) (result i32) (i32.atomic.rmw.cmpxchg offset=16 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i32.atomic.rmw8.cmpxchg") (param i64 i32 i32) (result i32) (i32.atomic.rmw8.cmpxchg_u offset=16 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i32.atomic.rmw16.cmpxchg") (param i64 i32 i32) (result i32) (i32.atomic.rmw16.cmpxchg_u offset=16 (local.get 0) (local.get 1) (local.get 2)))

    (func (export "i32.atomic.rmw.cmpxchg_2") (param i64 i32 i32) (result i32) (i32.atomic.rmw.cmpxchg offset=0x100000000 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i32.atomic.rmw8.cmpxchg_2") (param i64 i32 i32) (result i32) (i32.atomic.rmw8.cmpxchg_u offset=0x100000000 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i32.atomic.rmw16.cmpxchg_2") (param i64 i32 i32) (result i32) (i32.atomic.rmw16.cmpxchg_u offset=0x100000000 (local.get 0) (local.get 1) (local.get 2)))

    ;; i64 atomic Cmpxchg
    (func (export "i64.atomic.rmw.cmpxchg") (param i64 i64 i64) (result i64) (i64.atomic.rmw.cmpxchg offset=16 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i64.atomic.rmw8.cmpxchg") (param i64 i64 i64) (result i64) (i64.atomic.rmw8.cmpxchg_u offset=16 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i64.atomic.rmw16.cmpxchg") (param i64 i64 i64) (result i64) (i64.atomic.rmw16.cmpxchg_u offset=16 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i64.atomic.rmw32.cmpxchg") (param i64 i64 i64) (result i64) (i64.atomic.rmw32.cmpxchg_u offset=16 (local.get 0) (local.get 1) (local.get 2)))

    (func (export "i64.atomic.rmw.cmpxchg_2") (param i64 i64 i64) (result i64) (i64.atomic.rmw.cmpxchg offset=0x100000000 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i64.atomic.rmw8.cmpxchg_2") (param i64 i64 i64) (result i64) (i64.atomic.rmw8.cmpxchg_u offset=0x100000000 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i64.atomic.rmw16.cmpxchg_2") (param i64 i64 i64) (result i64) (i64.atomic.rmw16.cmpxchg_u offset=0x100000000 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i64.atomic.rmw32.cmpxchg_2") (param i64 i64 i64) (result i64) (i64.atomic.rmw32.cmpxchg_u offset=0x100000000 (local.get 0) (local.get 1) (local.get 2)))
)

;; i32
(assert_return (invoke "i32.store" (i64.const 0x10) (i32.const 1234)))
(assert_return (invoke "i32.load" (i64.const 0x10)) (i32.const 1234))
(assert_trap (invoke "i32.store" (i64.const 0x100000010) (i32.const 1234)) "out of bounds memory access")
(assert_trap (invoke "i32.load" (i64.const 0x100000010)) "out of bounds memory access")

(assert_return (invoke "i32.store_2" (i64.const 0x10) (i32.const 4321)))
(assert_return (invoke "i32.load_2" (i64.const 0x10)) (i32.const 4321))
(assert_trap (invoke "i32.store_2" (i64.const 0x100000010) (i32.const 4321)) "out of bounds memory access")
(assert_trap (invoke "i32.load_2" (i64.const 0x100000010)) "out of bounds memory access")

(assert_trap (invoke "i32.store_3" (i64.const 0x10) (i32.const 1234)) "out of bounds memory access")
(assert_trap (invoke "i32.load_3" (i64.const 0x10)) "out of bounds memory access")

;; i32_8
(assert_return (invoke "i32.store8" (i64.const 0x10) (i32.const 101)))
(assert_return (invoke "i32.load8u" (i64.const 0x10)) (i32.const 101))
(assert_return (invoke "i32.load8s" (i64.const 0x10)) (i32.const 101))
(assert_trap (invoke "i32.store8" (i64.const 0x100000000) (i32.const 101)) "out of bounds memory access")
(assert_trap (invoke "i32.load8u" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "i32.load8s" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i32.store8_2" (i64.const 0x10) (i32.const 101)) "out of bounds memory access")
(assert_trap (invoke "i32.load8u_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "i32.load8s_2" (i64.const 0x10)) "out of bounds memory access")

;; i32_16
(assert_return (invoke "i32.store16" (i64.const 0x10) (i32.const 32149)))
(assert_return (invoke "i32.load16u" (i64.const 0x10)) (i32.const 32149))
(assert_return (invoke "i32.load16s" (i64.const 0x10)) (i32.const 32149))
(assert_trap (invoke "i32.store16" (i64.const 0x100000000) (i32.const 32149)) "out of bounds memory access")
(assert_trap (invoke "i32.load16u" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "i32.load16s" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i32.store16_2" (i64.const 0x10) (i32.const 32149)) "out of bounds memory access")
(assert_trap (invoke "i32.load16u_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "i32.load16s_2" (i64.const 0x10)) "out of bounds memory access")

;; i64
(assert_return (invoke "i64.store" (i64.const 0x10) (i64.const 123456781234)))
(assert_return (invoke "i64.load" (i64.const 0x10)) (i64.const 123456781234))
(assert_trap (invoke "i64.store" (i64.const 0x100000010) (i64.const 123456781234)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const 0x100000010)) "out of bounds memory access")

(assert_return (invoke "i64.store_2" (i64.const 0x10) (i64.const 432187654321)))
(assert_return (invoke "i64.load_2" (i64.const 0x10)) (i64.const 432187654321))
(assert_trap (invoke "i64.store_2" (i64.const 0x100000010) (i64.const 432187654321)) "out of bounds memory access")
(assert_trap (invoke "i64.load_2" (i64.const 0x100000010)) "out of bounds memory access")

(assert_trap (invoke "i64.store_3" (i64.const 0x10) (i64.const 123456781234)) "out of bounds memory access")
(assert_trap (invoke "i64.load_3" (i64.const 0x10)) "out of bounds memory access")

;; i64_8
(assert_return (invoke "i64.store8" (i64.const 0x10) (i64.const 108)))
(assert_return (invoke "i64.load8u" (i64.const 0x10)) (i64.const 108))
(assert_return (invoke "i64.load8s" (i64.const 0x10)) (i64.const 108))
(assert_trap (invoke "i64.store8" (i64.const 0x100000000) (i64.const 108)) "out of bounds memory access")
(assert_trap (invoke "i64.load8u" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "i64.load8s" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i64.store8_2" (i64.const 0x10) (i64.const 108)) "out of bounds memory access")
(assert_trap (invoke "i64.load8u_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "i64.load8s_2" (i64.const 0x10)) "out of bounds memory access")

;; i64_16
(assert_return (invoke "i64.store16" (i64.const 0x10) (i64.const 30571)))
(assert_return (invoke "i64.load16u" (i64.const 0x10)) (i64.const 30571))
(assert_return (invoke "i64.load16s" (i64.const 0x10)) (i64.const 30571))
(assert_trap (invoke "i64.store16" (i64.const 0x100000000) (i64.const 30571)) "out of bounds memory access")
(assert_trap (invoke "i64.load16u" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "i64.load16s" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i64.store16_2" (i64.const 0x10) (i64.const 30571)) "out of bounds memory access")
(assert_trap (invoke "i64.load16u_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "i64.load16s_2" (i64.const 0x10)) "out of bounds memory access")

;; i64_32
(assert_return (invoke "i64.store32" (i64.const 0x10) (i64.const 2058934)))
(assert_return (invoke "i64.load32u" (i64.const 0x10)) (i64.const 2058934))
(assert_return (invoke "i64.load32s" (i64.const 0x10)) (i64.const 2058934))
(assert_trap (invoke "i64.store32" (i64.const 0x100000000) (i64.const 2058934)) "out of bounds memory access")
(assert_trap (invoke "i64.load32u" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "i64.load32s" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i64.store32_2" (i64.const 0x10) (i64.const 2058934)) "out of bounds memory access")
(assert_trap (invoke "i64.load32u_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "i64.load32s_2" (i64.const 0x10)) "out of bounds memory access")

;; f32
(assert_return (invoke "f32.store" (i64.const 0x10) (f32.const 1234.5)))
(assert_return (invoke "f32.load" (i64.const 0x10)) (f32.const 1234.5))
(assert_trap (invoke "f32.store" (i64.const 0x100000000) (f32.const 1234.5)) "out of bounds memory access")
(assert_trap (invoke "f32.load" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "f32.store_2" (i64.const 0x10) (f32.const 1234.5)) "out of bounds memory access")
(assert_trap (invoke "f32.load_2" (i64.const 0x10)) "out of bounds memory access")

;; f64
(assert_return (invoke "f64.store" (i64.const 0x10) (f64.const 123456789.5)))
(assert_return (invoke "f64.load" (i64.const 0x10)) (f64.const 123456789.5))
(assert_trap (invoke "f64.store" (i64.const 0x100000000) (f64.const 123456789.5)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "f64.store_2" (i64.const 0x10) (f64.const 123456789.5)) "out of bounds memory access")
(assert_trap (invoke "f64.load_2" (i64.const 0x10)) "out of bounds memory access")

;; v128
(assert_return (invoke "v128.store" (i64.const 0x10) (v128.const i64x2 1 2)))
(assert_return (invoke "v128.load" (i64.const 0x10)) (v128.const i64x2 1 2))
(assert_trap (invoke "v128.store" (i64.const 0x100000000) (v128.const i64x2 1 2)) "out of bounds memory access")
(assert_trap (invoke "v128.load" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.store_2" (i64.const 0x10) (v128.const i64x2 1 2)) "out of bounds memory access")
(assert_trap (invoke "v128.load_2" (i64.const 0x10)) "out of bounds memory access")

;; clear mem
(assert_return (invoke "v128.store" (i64.const 0x10) (v128.const i64x2 0 0)))

;; v128 8x8
(assert_return (invoke "v128.load8x8s" (i64.const 0x10)) (v128.const i64x2 0 0))
(assert_return (invoke "v128.load8x8u" (i64.const 0x10)) (v128.const i64x2 0 0))

(assert_trap (invoke "v128.load8x8s" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "v128.load8x8u" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.load8x8s_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "v128.load8x8u_2" (i64.const 0x10)) "out of bounds memory access")

;; v128 16x4
(assert_return (invoke "v128.load16x4s" (i64.const 0x10)) (v128.const i64x2 0 0))
(assert_return (invoke "v128.load16x4u" (i64.const 0x10)) (v128.const i64x2 0 0))

(assert_trap (invoke "v128.load16x4s" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "v128.load16x4u" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.load16x4s_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "v128.load16x4u_2" (i64.const 0x10)) "out of bounds memory access")

;; v128 32x2
(assert_return (invoke "v128.load32x2s" (i64.const 0x10)) (v128.const i64x2 0 0))
(assert_return (invoke "v128.load32x2u" (i64.const 0x10)) (v128.const i64x2 0 0))

(assert_trap (invoke "v128.load32x2s" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "v128.load32x2u" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.load32x2s_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "v128.load32x2u_2" (i64.const 0x10)) "out of bounds memory access")

;; v128 splat
(assert_return (invoke "v128.load8splat" (i64.const 0x10)) (v128.const i64x2 0 0))
(assert_return (invoke "v128.load16splat" (i64.const 0x10)) (v128.const i64x2 0 0))
(assert_return (invoke "v128.load32splat" (i64.const 0x10)) (v128.const i64x2 0 0))
(assert_return (invoke "v128.load64splat" (i64.const 0x10)) (v128.const i64x2 0 0))

(assert_trap (invoke "v128.load8splat" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "v128.load16splat" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "v128.load32splat" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "v128.load64splat" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.load8splat_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "v128.load16splat_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "v128.load32splat_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "v128.load64splat_2" (i64.const 0x10)) "out of bounds memory access")

;; v128 lane 8
(assert_return (invoke "v128.store8lane" (i64.const 0x10) (v128.const i8x16 75 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
(assert_return (invoke "v128.load8lane" (i64.const 0x10)) (v128.const i8x16 75 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0))

(assert_trap (invoke "v128.store8lane" (i64.const 0x100000000) (v128.const i64x2 0 0)) "out of bounds memory access")
(assert_trap (invoke "v128.load8lane" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.store8lane_2" (i64.const 0x10) (v128.const i64x2 0 0)) "out of bounds memory access")
(assert_trap (invoke "v128.load8lane_2" (i64.const 0x10)) "out of bounds memory access")

;; v128 lane 16
(assert_return (invoke "v128.store16lane" (i64.const 0x10) (v128.const i16x8 30678 0 0 0 0 0 0 0)))
(assert_return (invoke "v128.load16lane" (i64.const 0x10)) (v128.const i16x8 30678 0 0 0 0 0 0 0))

(assert_trap (invoke "v128.store16lane" (i64.const 0x100000000) (v128.const i64x2 0 0)) "out of bounds memory access")
(assert_trap (invoke "v128.load16lane" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.store16lane_2" (i64.const 0x10) (v128.const i64x2 0 0)) "out of bounds memory access")
(assert_trap (invoke "v128.load16lane_2" (i64.const 0x10)) "out of bounds memory access")

;; v128 lane 32
(assert_return (invoke "v128.store32lane" (i64.const 0x10) (v128.const i32x4 2180785617 0 0 0)))
(assert_return (invoke "v128.load32lane" (i64.const 0x10)) (v128.const i32x4 2180785617 0 0 0))

(assert_trap (invoke "v128.store32lane" (i64.const 0x100000000) (v128.const i64x2 0 0)) "out of bounds memory access")
(assert_trap (invoke "v128.load32lane" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.store32lane_2" (i64.const 0x10) (v128.const i64x2 0 0)) "out of bounds memory access")
(assert_trap (invoke "v128.load32lane_2" (i64.const 0x10)) "out of bounds memory access")

;; v128 lane 64
(assert_return (invoke "v128.store64lane" (i64.const 0x10) (v128.const i64x2 314756237046123789 0)))
(assert_return (invoke "v128.load64lane" (i64.const 0x10)) (v128.const i64x2 314756237046123789 0))

(assert_trap (invoke "v128.store64lane" (i64.const 0x100000000) (v128.const i64x2 0 0)) "out of bounds memory access")
(assert_trap (invoke "v128.load64lane" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.store64lane_2" (i64.const 0x10) (v128.const i64x2 0 0)) "out of bounds memory access")
(assert_trap (invoke "v128.load64lane_2" (i64.const 0x10)) "out of bounds memory access")

;; v128 load zero
(assert_return (invoke "i32.store_2" (i64.const 0x10) (i32.const 1579840812)))
(assert_return (invoke "v128.load32zero" (i64.const 0x10)) (v128.const i32x4 1579840812 0 0 0))
(assert_return (invoke "i64.store_2" (i64.const 0x10) (i64.const 84592345257645125)))
(assert_return (invoke "v128.load64zero" (i64.const 0x10)) (v128.const i64x2 84592345257645125 0))

(assert_trap (invoke "v128.load32zero" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "v128.load64zero" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.load32zero_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "v128.load64zero_2" (i64.const 0x10)) "out of bounds memory access")

;; atomic core
(assert_return (invoke "atomic.notify" (i64.const 0x10)))
(assert_return (invoke "memory.atomic.wait32" (i64.const 0x10)))
(assert_return (invoke "memory.atomic.wait64" (i64.const 0x10)))

(assert_trap (invoke "atomic.notify" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "memory.atomic.wait32" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "memory.atomic.wait64" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "atomic.notify_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "memory.atomic.wait32_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "memory.atomic.wait64_2" (i64.const 0x10)) "out of bounds memory access")

;; i32 atomic
(assert_return (invoke "i32.atomic.store" (i64.const 0x10) (i32.const 1480532187)))
(assert_return (invoke "i32.atomic.load" (i64.const 0x10)) (i32.const 1480532187))

(assert_trap (invoke "i32.atomic.store" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.load" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i32.atomic.store_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.load_2" (i64.const 0x10)) "out of bounds memory access")

;; i32 atomic 8
(assert_return (invoke "i32.atomic.store8" (i64.const 0x10) (i32.const 245)))
(assert_return (invoke "i32.atomic.load8" (i64.const 0x10)) (i32.const 245))

(assert_trap (invoke "i32.atomic.store8" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.load8" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i32.atomic.store8_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.load8_2" (i64.const 0x10)) "out of bounds memory access")

;; i32 atomic 16
(assert_return (invoke "i32.atomic.store16" (i64.const 0x10) (i32.const 63416)))
(assert_return (invoke "i32.atomic.load16" (i64.const 0x10)) (i32.const 63416))

(assert_trap (invoke "i32.atomic.store16" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.load16" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i32.atomic.store16_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.load16_2" (i64.const 0x10)) "out of bounds memory access")

;; i64 atomic
(assert_return (invoke "i64.atomic.store" (i64.const 0x10) (i64.const 8923601264510235)))
(assert_return (invoke "i64.atomic.load" (i64.const 0x10)) (i64.const 8923601264510235))

(assert_trap (invoke "i64.atomic.store" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.load" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.store_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.load_2" (i64.const 0x10)) "out of bounds memory access")

;; i64 atomic 8
(assert_return (invoke "i64.atomic.store8" (i64.const 0x10) (i64.const 208)))
(assert_return (invoke "i64.atomic.load8" (i64.const 0x10)) (i64.const 208))

(assert_trap (invoke "i64.atomic.store8" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.load8" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.store8_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.load8_2" (i64.const 0x10)) "out of bounds memory access")

;; i64 atomic 16
(assert_return (invoke "i64.atomic.store16" (i64.const 0x10) (i64.const 58930)))
(assert_return (invoke "i64.atomic.load16" (i64.const 0x10)) (i64.const 58930))

(assert_trap (invoke "i64.atomic.store16" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.load16" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.store16_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.load16_2" (i64.const 0x10)) "out of bounds memory access")

;; i64 atomic 32
(assert_return (invoke "i64.atomic.store32" (i64.const 0x10) (i64.const 4035164968)))
(assert_return (invoke "i64.atomic.load32" (i64.const 0x10)) (i64.const 4035164968))

(assert_trap (invoke "i64.atomic.store32" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.load32" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.store32_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.load32_2" (i64.const 0x10)) "out of bounds memory access")

;; i32 atomic rmw
(assert_return (invoke "i32.store_2" (i64.const 0x10) (i32.const 0)))
(assert_return (invoke "i32.atomic.rmw.add" (i64.const 0x10) (i32.const 1684902794)) (i32.const 0))
(assert_return (invoke "i32.atomic.rmw.sub" (i64.const 0x10) (i32.const 1684902794)) (i32.const 1684902794))

(assert_trap (invoke "i32.atomic.rmw.add" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw.sub" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")

(assert_trap (invoke "i32.atomic.rmw.add_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw.sub_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")

;; i32 atomic rmw 8
(assert_return (invoke "i32.store_2" (i64.const 0x10) (i32.const 0)))
(assert_return (invoke "i32.atomic.rmw8.or" (i64.const 0x10) (i32.const 159)) (i32.const 0))
(assert_return (invoke "i32.atomic.rmw8.xor" (i64.const 0x10) (i32.const 159)) (i32.const 159))

(assert_trap (invoke "i32.atomic.rmw8.or" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw8.xor" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")

(assert_trap (invoke "i32.atomic.rmw8.or_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw8.xor_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")

;; i32 atomic rmw 16
(assert_return (invoke "i32.store_2" (i64.const 0x10) (i32.const 0)))
(assert_return (invoke "i32.atomic.rmw16.or" (i64.const 0x10) (i32.const 159)) (i32.const 0))
(assert_return (invoke "i32.atomic.rmw16.and" (i64.const 0x10) (i32.const 159)) (i32.const 159))

(assert_trap (invoke "i32.atomic.rmw16.or" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw16.and" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")

(assert_trap (invoke "i32.atomic.rmw16.or_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw16.and_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")

;; i64 atomic rmw
(assert_return (invoke "i64.store_2" (i64.const 0x10) (i64.const 0)))
(assert_return (invoke "i64.atomic.rmw.add" (i64.const 0x10) (i64.const 194602756123704341)) (i64.const 0))
(assert_return (invoke "i64.atomic.rmw.sub" (i64.const 0x10) (i64.const 194602756123704341)) (i64.const 194602756123704341))

(assert_trap (invoke "i64.atomic.rmw.add" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw.sub" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.rmw.add_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw.sub_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")

;; i64 atomic rmw 8
(assert_return (invoke "i64.store_2" (i64.const 0x10) (i64.const 0)))
(assert_return (invoke "i64.atomic.rmw8.or" (i64.const 0x10) (i64.const 180)) (i64.const 0))
(assert_return (invoke "i64.atomic.rmw8.xor" (i64.const 0x10) (i64.const 180)) (i64.const 180))

(assert_trap (invoke "i64.atomic.rmw8.or" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw8.xor" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.rmw8.or_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw8.xor_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")

;; i64 atomic rmw 16
(assert_return (invoke "i64.store_2" (i64.const 0x10) (i64.const 0)))
(assert_return (invoke "i64.atomic.rmw16.or" (i64.const 0x10) (i64.const 59468)) (i64.const 0))
(assert_return (invoke "i64.atomic.rmw16.and" (i64.const 0x10) (i64.const 59468)) (i64.const 59468))

(assert_trap (invoke "i64.atomic.rmw16.or" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw16.and" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.rmw16.or_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw16.and_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")

;; i64 atomic rmw 32
(assert_return (invoke "i64.store_2" (i64.const 0x10) (i64.const 0)))
(assert_return (invoke "i64.atomic.rmw32.add" (i64.const 0x10) (i64.const 1962460524)) (i64.const 0))
(assert_return (invoke "i64.atomic.rmw32.and" (i64.const 0x10) (i64.const 1962460524)) (i64.const 1962460524))

(assert_trap (invoke "i64.atomic.rmw32.add" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw32.and" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.rmw32.add_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw32.and_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")

;; i32 atomic Xchg
(assert_return (invoke "i32.store_2" (i64.const 0x10) (i32.const 1683920478)))
(assert_return (invoke "i32.atomic.rmw.xchg" (i64.const 0x10) (i32.const 0)) (i32.const 1683920478))
(assert_return (invoke "i32.store8" (i64.const 0x10) (i32.const 238)))
(assert_return (invoke "i32.atomic.rmw8.xchg" (i64.const 0x10) (i32.const 0)) (i32.const 238))
(assert_return (invoke "i32.store16" (i64.const 0x10) (i32.const 57840)))
(assert_return (invoke "i32.atomic.rmw16.xchg" (i64.const 0x10) (i32.const 0)) (i32.const 57840))

(assert_trap (invoke "i32.atomic.rmw.xchg" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw8.xchg" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw16.xchg" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")

(assert_trap (invoke "i32.atomic.rmw.xchg_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw8.xchg_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw16.xchg_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")

;; i64 atomic Xchg
(assert_return (invoke "i64.store_2" (i64.const 0x10) (i64.const 7686420451258)))
(assert_return (invoke "i64.atomic.rmw.xchg" (i64.const 0x10) (i64.const 0)) (i64.const 7686420451258))
(assert_return (invoke "i64.store8" (i64.const 0x10) (i64.const 226)))
(assert_return (invoke "i64.atomic.rmw8.xchg" (i64.const 0x10) (i64.const 0)) (i64.const 226))
(assert_return (invoke "i64.store16" (i64.const 0x10) (i64.const 53651)))
(assert_return (invoke "i64.atomic.rmw16.xchg" (i64.const 0x10) (i64.const 0)) (i64.const 53651))
(assert_return (invoke "i64.store32" (i64.const 0x10) (i64.const 1687254093)))
(assert_return (invoke "i64.atomic.rmw32.xchg" (i64.const 0x10) (i64.const 0)) (i64.const 1687254093))

(assert_trap (invoke "i64.atomic.rmw.xchg" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw8.xchg" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw16.xchg" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw32.xchg" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.rmw.xchg_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw8.xchg_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw16.xchg_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw32.xchg_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")

;; i32 atomic Cmpxchg
(assert_return (invoke "i32.store_2" (i64.const 0x10) (i32.const 1809274815)))
(assert_return (invoke "i32.atomic.rmw.cmpxchg" (i64.const 0x10) (i32.const 1809274815) (i32.const 0)) (i32.const 1809274815))
(assert_return (invoke "i32.store8" (i64.const 0x10) (i32.const 164)))
(assert_return (invoke "i32.atomic.rmw8.cmpxchg" (i64.const 0x10) (i32.const 164) (i32.const 0)) (i32.const 164))
(assert_return (invoke "i32.store16" (i64.const 0x10) (i32.const 59218)))
(assert_return (invoke "i32.atomic.rmw16.cmpxchg" (i64.const 0x10) (i32.const 59218) (i32.const 0)) (i32.const 59218))

(assert_trap (invoke "i32.atomic.rmw.cmpxchg" (i64.const 0x100000000) (i32.const 0) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw8.cmpxchg" (i64.const 0x100000000) (i32.const 0) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw16.cmpxchg" (i64.const 0x100000000) (i32.const 0) (i32.const 0)) "out of bounds memory access")

(assert_trap (invoke "i32.atomic.rmw.cmpxchg_2" (i64.const 0x10) (i32.const 0) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw8.cmpxchg_2" (i64.const 0x10) (i32.const 0) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw16.cmpxchg_2" (i64.const 0x10) (i32.const 0) (i32.const 0)) "out of bounds memory access")

;; i64 atomic Cmpxchg
(assert_return (invoke "i64.store_2" (i64.const 0x10) (i64.const 127857628907652)))
(assert_return (invoke "i64.atomic.rmw.cmpxchg" (i64.const 0x10) (i64.const 127857628907652) (i64.const 0)) (i64.const 127857628907652))
(assert_return (invoke "i64.store8" (i64.const 0x10) (i64.const 201)))
(assert_return (invoke "i64.atomic.rmw8.cmpxchg" (i64.const 0x10) (i64.const 201) (i64.const 0)) (i64.const 201))
(assert_return (invoke "i64.store16" (i64.const 0x10) (i64.const 60437)))
(assert_return (invoke "i64.atomic.rmw16.cmpxchg" (i64.const 0x10) (i64.const 60437) (i64.const 0)) (i64.const 60437))
(assert_return (invoke "i64.store32" (i64.const 0x10) (i64.const 2067381469)))
(assert_return (invoke "i64.atomic.rmw32.cmpxchg" (i64.const 0x10) (i64.const 2067381469) (i64.const 0)) (i64.const 2067381469))

(assert_trap (invoke "i64.atomic.rmw.cmpxchg" (i64.const 0x100000000) (i64.const 0) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw8.cmpxchg" (i64.const 0x100000000) (i64.const 0) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw16.cmpxchg" (i64.const 0x100000000) (i64.const 0) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw32.cmpxchg" (i64.const 0x100000000) (i64.const 0) (i64.const 0)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.rmw.cmpxchg_2" (i64.const 0x10) (i64.const 0) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw8.cmpxchg_2" (i64.const 0x10) (i64.const 0) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw16.cmpxchg_2" (i64.const 0x10) (i64.const 0) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw32.cmpxchg_2" (i64.const 0x10) (i64.const 0) (i64.const 0)) "out of bounds memory access")

(module
    (memory i64 0)
    (memory $mem i64 1)

    ;; i32
    (func (export "i32.store") (param i64 i32) (i32.store $mem (local.get 0) (local.get 1)))
    (func (export "i32.load") (param i64) (result i32) (i32.load $mem (local.get 0)))

    (func (export "i32.store_2") (param i64 i32) (i32.store $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.load_2") (param i64) (result i32) (i32.load $mem offset=16 (local.get 0)))

    (func (export "i32.store_3") (param i64 i32) (i32.store $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.load_3") (param i64) (result i32) (i32.load $mem offset=0x100000000 (local.get 0)))

    ;; i32_8
    (func (export "i32.store8") (param i64 i32) (i32.store8 $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.load8u") (param i64) (result i32) (i32.load8_u $mem offset=16 (local.get 0)))
    (func (export "i32.load8s") (param i64) (result i32) (i32.load8_s $mem offset=16 (local.get 0)))

    (func (export "i32.store8_2") (param i64 i32) (i32.store8 $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.load8u_2") (param i64) (result i32) (i32.load8_u $mem offset=0x100000000 (local.get 0)))
    (func (export "i32.load8s_2") (param i64) (result i32) (i32.load8_s $mem offset=0x100000000 (local.get 0)))

    ;; i32_16
    (func (export "i32.store16") (param i64 i32) (i32.store16 $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.load16u") (param i64) (result i32) (i32.load16_u $mem offset=16 (local.get 0)))
    (func (export "i32.load16s") (param i64) (result i32) (i32.load16_s $mem offset=16 (local.get 0)))

    (func (export "i32.store16_2") (param i64 i32) (i32.store16 $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.load16u_2") (param i64) (result i32) (i32.load16_u $mem offset=0x100000000 (local.get 0)))
    (func (export "i32.load16s_2") (param i64) (result i32) (i32.load16_s $mem offset=0x100000000 (local.get 0)))

    ;; i64
    (func (export "i64.store") (param i64 i64) (i64.store $mem (local.get 0) (local.get 1)))
    (func (export "i64.load") (param i64) (result i64) (i64.load $mem (local.get 0)))

    (func (export "i64.store_2") (param i64 i64) (i64.store $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.load_2") (param i64) (result i64) (i64.load $mem offset=16 (local.get 0)))

    (func (export "i64.store_3") (param i64 i64) (i64.store $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.load_3") (param i64) (result i64) (i64.load $mem offset=0x100000000 (local.get 0)))

    ;; i64_8
    (func (export "i64.store8") (param i64 i64) (i64.store8 $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.load8u") (param i64) (result i64) (i64.load8_u $mem offset=16 (local.get 0)))
    (func (export "i64.load8s") (param i64) (result i64) (i64.load8_s $mem offset=16 (local.get 0)))

    (func (export "i64.store8_2") (param i64 i64) (i64.store8 $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.load8u_2") (param i64) (result i64) (i64.load8_u $mem offset=0x100000000 (local.get 0)))
    (func (export "i64.load8s_2") (param i64) (result i64) (i64.load8_s $mem offset=0x100000000 (local.get 0)))

    ;; i64_16
    (func (export "i64.store16") (param i64 i64) (i64.store16 $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.load16u") (param i64) (result i64) (i64.load16_u $mem offset=16 (local.get 0)))
    (func (export "i64.load16s") (param i64) (result i64) (i64.load16_s $mem offset=16 (local.get 0)))

    (func (export "i64.store16_2") (param i64 i64) (i64.store16 $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.load16u_2") (param i64) (result i64) (i64.load16_u $mem offset=0x100000000 (local.get 0)))
    (func (export "i64.load16s_2") (param i64) (result i64) (i64.load16_s $mem offset=0x100000000 (local.get 0)))

    ;; i64_32
    (func (export "i64.store32") (param i64 i64) (i64.store32 $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.load32u") (param i64) (result i64) (i64.load32_u $mem offset=16 (local.get 0)))
    (func (export "i64.load32s") (param i64) (result i64) (i64.load32_s $mem offset=16 (local.get 0)))

    (func (export "i64.store32_2") (param i64 i64) (i64.store32 $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.load32u_2") (param i64) (result i64) (i64.load32_u $mem offset=0x100000000 (local.get 0)))
    (func (export "i64.load32s_2") (param i64) (result i64) (i64.load32_s $mem offset=0x100000000 (local.get 0)))

    ;; f32
    (func (export "f32.store") (param i64 f32) (f32.store $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "f32.load") (param i64) (result f32) (f32.load $mem offset=16 (local.get 0)))

    (func (export "f32.store_2") (param i64 f32) (f32.store $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "f32.load_2") (param i64) (result f32) (f32.load $mem offset=0x100000000 (local.get 0)))

    ;; f64
    (func (export "f64.store") (param i64 f64) (f64.store $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "f64.load") (param i64) (result f64) (f64.load $mem offset=16 (local.get 0)))

    (func (export "f64.store_2") (param i64 f64) (f64.store $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "f64.load_2") (param i64) (result f64) (f64.load $mem offset=0x100000000 (local.get 0)))

    ;; v128
    (func (export "v128.store") (param i64 v128) (v128.store $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "v128.load") (param i64) (result v128) (v128.load $mem offset=16 (local.get 0)))

    (func (export "v128.store_2") (param i64 v128) (v128.store $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "v128.load_2") (param i64) (result v128) (v128.load $mem offset=0x100000000 (local.get 0)))

    ;; v128 8x8
    (func (export "v128.load8x8s") (param i64) (result v128) (v128.load8x8_s $mem offset=16 (local.get 0)))
    (func (export "v128.load8x8u") (param i64) (result v128) (v128.load8x8_u $mem offset=16 (local.get 0)))

    (func (export "v128.load8x8s_2") (param i64) (result v128) (v128.load8x8_s $mem offset=0x100000000 (local.get 0)))
    (func (export "v128.load8x8u_2") (param i64) (result v128) (v128.load8x8_u $mem offset=0x100000000 (local.get 0)))

    ;; v128 16x4
    (func (export "v128.load16x4s") (param i64) (result v128) (v128.load16x4_s $mem offset=16 (local.get 0)))
    (func (export "v128.load16x4u") (param i64) (result v128) (v128.load16x4_u $mem offset=16 (local.get 0)))

    (func (export "v128.load16x4s_2") (param i64) (result v128) (v128.load16x4_s $mem offset=0x100000000 (local.get 0)))
    (func (export "v128.load16x4u_2") (param i64) (result v128) (v128.load16x4_u $mem offset=0x100000000 (local.get 0)))

    ;; v128 32x2
    (func (export "v128.load32x2s") (param i64) (result v128) (v128.load32x2_s $mem offset=16 (local.get 0)))
    (func (export "v128.load32x2u") (param i64) (result v128) (v128.load32x2_u $mem offset=16 (local.get 0)))

    (func (export "v128.load32x2s_2") (param i64) (result v128) (v128.load32x2_s $mem offset=0x100000000 (local.get 0)))
    (func (export "v128.load32x2u_2") (param i64) (result v128) (v128.load32x2_u $mem offset=0x100000000 (local.get 0)))

    ;; v128 splat
    (func (export "v128.load8splat") (param i64) (result v128) (v128.load8_splat $mem offset=16 (local.get 0)))
    (func (export "v128.load16splat") (param i64) (result v128) (v128.load16_splat $mem offset=16 (local.get 0)))
    (func (export "v128.load32splat") (param i64) (result v128) (v128.load32_splat $mem offset=16 (local.get 0)))
    (func (export "v128.load64splat") (param i64) (result v128) (v128.load64_splat $mem offset=16 (local.get 0)))

    (func (export "v128.load8splat_2") (param i64) (result v128) (v128.load8_splat $mem offset=0x100000000 (local.get 0)))
    (func (export "v128.load16splat_2") (param i64) (result v128) (v128.load16_splat $mem offset=0x100000000 (local.get 0)))
    (func (export "v128.load32splat_2") (param i64) (result v128) (v128.load32_splat $mem offset=0x100000000 (local.get 0)))
    (func (export "v128.load64splat_2") (param i64) (result v128) (v128.load64_splat $mem offset=0x100000000 (local.get 0)))

    ;; v128 lane 8
    (func (export "v128.store8lane") (param i64 v128) (v128.store8_lane $mem offset=16 0 (local.get 0) (local.get 1)))
    (func (export "v128.load8lane") (param i64) (result v128) (v128.load8_lane $mem offset=16 0 (local.get 0) (v128.const i64x2 0 0)))

    (func (export "v128.store8lane_2") (param i64 v128) (v128.store8_lane $mem offset=0x100000000 0 (local.get 0) (local.get 1)))
    (func (export "v128.load8lane_2") (param i64) (result v128) (v128.load8_lane $mem offset=0x100000000 0 (local.get 0) (v128.const i64x2 0 0)))

    ;; v128 lane 16
    (func (export "v128.store16lane") (param i64 v128) (v128.store16_lane $mem offset=16 0 (local.get 0) (local.get 1)))
    (func (export "v128.load16lane") (param i64) (result v128) (v128.load16_lane $mem offset=16 0 (local.get 0) (v128.const i64x2 0 0)))

    (func (export "v128.store16lane_2") (param i64 v128) (v128.store16_lane $mem offset=0x100000000 0 (local.get 0) (local.get 1)))
    (func (export "v128.load16lane_2") (param i64) (result v128) (v128.load16_lane $mem offset=0x100000000 0 (local.get 0) (v128.const i64x2 0 0)))

    ;; v128 lane 32
    (func (export "v128.store32lane") (param i64 v128) (v128.store32_lane $mem offset=16 0 (local.get 0) (local.get 1)))
    (func (export "v128.load32lane") (param i64) (result v128) (v128.load32_lane $mem offset=16 0 (local.get 0) (v128.const i64x2 0 0)))

    (func (export "v128.store32lane_2") (param i64 v128) (v128.store32_lane $mem offset=0x100000000 0 (local.get 0) (local.get 1)))
    (func (export "v128.load32lane_2") (param i64) (result v128) (v128.load32_lane $mem offset=0x100000000 0 (local.get 0) (v128.const i64x2 0 0)))

    ;; v128 lane 64
    (func (export "v128.store64lane") (param i64 v128) (v128.store64_lane $mem offset=16 0 (local.get 0) (local.get 1)))
    (func (export "v128.load64lane") (param i64) (result v128) (v128.load64_lane $mem offset=16 0 (local.get 0) (v128.const i64x2 0 0)))

    (func (export "v128.store64lane_2") (param i64 v128) (v128.store64_lane $mem offset=0x100000000 0 (local.get 0) (local.get 1)))
    (func (export "v128.load64lane_2") (param i64) (result v128) (v128.load64_lane $mem offset=0x100000000 0 (local.get 0) (v128.const i64x2 0 0)))

    ;; v128 load zero
    (func (export "v128.load32zero") (param i64) (result v128) (v128.load32_zero $mem offset=16 (local.get 0)))
    (func (export "v128.load64zero") (param i64) (result v128) (v128.load64_zero $mem offset=16 (local.get 0)))

    (func (export "v128.load32zero_2") (param i64) (result v128) (v128.load32_zero $mem offset=0x100000000 (local.get 0)))
    (func (export "v128.load64zero_2") (param i64) (result v128) (v128.load64_zero $mem offset=0x100000000 (local.get 0)))

    ;; atomic core
    (func (export "atomic.notify") (param i64) (drop (memory.atomic.notify $mem offset=16 (local.get 0) (i32.const 0))))
    (func (export "memory.atomic.wait32") (param i64) (drop (memory.atomic.wait32 $mem offset=16 (local.get 0) (i32.const 0) (i64.const 0))))
    (func (export "memory.atomic.wait64") (param i64) (drop (memory.atomic.wait64 $mem offset=16 (local.get 0) (i64.const 0) (i64.const 0))))

    (func (export "atomic.notify_2") (param i64) (drop (memory.atomic.notify $mem offset=0x100000000 (local.get 0) (i32.const 0))))
    (func (export "memory.atomic.wait32_2") (param i64) (drop (memory.atomic.wait32 $mem offset=0x100000000 (local.get 0) (i32.const 0) (i64.const 0))))
    (func (export "memory.atomic.wait64_2") (param i64) (drop (memory.atomic.wait64 $mem offset=0x100000000 (local.get 0) (i64.const 0) (i64.const 0))))

    ;; i32 atomic
    (func (export "i32.atomic.store") (param i64 i32) (i32.atomic.store $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.load") (param i64) (result i32) (i32.atomic.load $mem offset=16 (local.get 0)))

    (func (export "i32.atomic.store_2") (param i64 i32) (i32.atomic.store $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.load_2") (param i64) (result i32) (i32.atomic.load $mem offset=0x100000000 (local.get 0)))

    ;; i32 atomic 8
    (func (export "i32.atomic.store8") (param i64 i32) (i32.atomic.store8 $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.load8") (param i64) (result i32) (i32.atomic.load8_u $mem offset=16 (local.get 0)))

    (func (export "i32.atomic.store8_2") (param i64 i32) (i32.atomic.store8 $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.load8_2") (param i64) (result i32) (i32.atomic.load8_u $mem offset=0x100000000 (local.get 0)))

    ;; i32 atomic 16
    (func (export "i32.atomic.store16") (param i64 i32) (i32.atomic.store16 $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.load16") (param i64) (result i32) (i32.atomic.load16_u $mem offset=16 (local.get 0)))

    (func (export "i32.atomic.store16_2") (param i64 i32) (i32.atomic.store16 $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.load16_2") (param i64) (result i32) (i32.atomic.load16_u $mem offset=0x100000000 (local.get 0)))

    ;; i64 atomic
    (func (export "i64.atomic.store") (param i64 i64) (i64.atomic.store $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.load") (param i64) (result i64) (i64.atomic.load $mem offset=16 (local.get 0)))

    (func (export "i64.atomic.store_2") (param i64 i64) (i64.atomic.store $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.load_2") (param i64) (result i64) (i64.atomic.load $mem offset=0x100000000 (local.get 0)))

    ;; i64 atomic 8
    (func (export "i64.atomic.store8") (param i64 i64) (i64.atomic.store8 $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.load8") (param i64) (result i64) (i64.atomic.load8_u $mem offset=16 (local.get 0)))

    (func (export "i64.atomic.store8_2") (param i64 i64) (i64.atomic.store8 $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.load8_2") (param i64) (result i64) (i64.atomic.load8_u $mem offset=0x100000000 (local.get 0)))

    ;; i64 atomic 16
    (func (export "i64.atomic.store16") (param i64 i64) (i64.atomic.store16 $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.load16") (param i64) (result i64) (i64.atomic.load16_u $mem offset=16 (local.get 0)))

    (func (export "i64.atomic.store16_2") (param i64 i64) (i64.atomic.store16 $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.load16_2") (param i64) (result i64) (i64.atomic.load16_u $mem offset=0x100000000 (local.get 0)))

    ;; i64 atomic 32
    (func (export "i64.atomic.store32") (param i64 i64) (i64.atomic.store32 $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.load32") (param i64) (result i64) (i64.atomic.load32_u $mem offset=16 (local.get 0)))

    (func (export "i64.atomic.store32_2") (param i64 i64) (i64.atomic.store32 $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.load32_2") (param i64) (result i64) (i64.atomic.load32_u $mem offset=0x100000000 (local.get 0)))

    ;; i32 atomic rmw
    (func (export "i32.atomic.rmw.add") (param i64 i32) (result i32) (i32.atomic.rmw.add $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw.sub") (param i64 i32) (result i32) (i32.atomic.rmw.sub $mem offset=16 (local.get 0) (local.get 1)))

    (func (export "i32.atomic.rmw.add_2") (param i64 i32) (result i32) (i32.atomic.rmw.add $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw.sub_2") (param i64 i32) (result i32) (i32.atomic.rmw.sub $mem offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i32 atomic rmw 8
    (func (export "i32.atomic.rmw8.or") (param i64 i32) (result i32) (i32.atomic.rmw8.or_u $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw8.xor") (param i64 i32) (result i32) (i32.atomic.rmw8.xor_u $mem offset=16 (local.get 0) (local.get 1)))

    (func (export "i32.atomic.rmw8.or_2") (param i64 i32) (result i32) (i32.atomic.rmw8.or_u $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw8.xor_2") (param i64 i32) (result i32) (i32.atomic.rmw8.xor_u $mem offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i32 atomic rmw 16
    (func (export "i32.atomic.rmw16.or") (param i64 i32) (result i32) (i32.atomic.rmw16.or_u $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw16.and") (param i64 i32) (result i32) (i32.atomic.rmw16.and_u $mem offset=16 (local.get 0) (local.get 1)))

    (func (export "i32.atomic.rmw16.or_2") (param i64 i32) (result i32) (i32.atomic.rmw16.or_u $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw16.and_2") (param i64 i32) (result i32) (i32.atomic.rmw16.and_u $mem offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i64 atomic rmw
    (func (export "i64.atomic.rmw.add") (param i64 i64) (result i64) (i64.atomic.rmw.add $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw.sub") (param i64 i64) (result i64) (i64.atomic.rmw.sub $mem offset=16 (local.get 0) (local.get 1)))

    (func (export "i64.atomic.rmw.add_2") (param i64 i64) (result i64) (i64.atomic.rmw.add $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw.sub_2") (param i64 i64) (result i64) (i64.atomic.rmw.sub $mem offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i64 atomic rmw 8
    (func (export "i64.atomic.rmw8.or") (param i64 i64) (result i64) (i64.atomic.rmw8.or_u $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw8.xor") (param i64 i64) (result i64) (i64.atomic.rmw8.xor_u $mem offset=16 (local.get 0) (local.get 1)))

    (func (export "i64.atomic.rmw8.or_2") (param i64 i64) (result i64) (i64.atomic.rmw8.or_u $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw8.xor_2") (param i64 i64) (result i64) (i64.atomic.rmw8.xor_u $mem offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i64 atomic rmw 16
    (func (export "i64.atomic.rmw16.or") (param i64 i64) (result i64) (i64.atomic.rmw16.or_u $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw16.and") (param i64 i64) (result i64) (i64.atomic.rmw16.and_u $mem offset=16 (local.get 0) (local.get 1)))

    (func (export "i64.atomic.rmw16.or_2") (param i64 i64) (result i64) (i64.atomic.rmw16.or_u $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw16.and_2") (param i64 i64) (result i64) (i64.atomic.rmw16.and_u $mem offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i64 atomic rmw 32
    (func (export "i64.atomic.rmw32.add") (param i64 i64) (result i64) (i64.atomic.rmw32.add_u $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw32.and") (param i64 i64) (result i64) (i64.atomic.rmw32.and_u $mem offset=16 (local.get 0) (local.get 1)))

    (func (export "i64.atomic.rmw32.add_2") (param i64 i64) (result i64) (i64.atomic.rmw32.add_u $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw32.and_2") (param i64 i64) (result i64) (i64.atomic.rmw32.and_u $mem offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i32 atomic Xchg
    (func (export "i32.atomic.rmw.xchg") (param i64 i32) (result i32) (i32.atomic.rmw.xchg $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw8.xchg") (param i64 i32) (result i32) (i32.atomic.rmw8.xchg_u $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw16.xchg") (param i64 i32) (result i32) (i32.atomic.rmw16.xchg_u $mem offset=16 (local.get 0) (local.get 1)))

    (func (export "i32.atomic.rmw.xchg_2") (param i64 i32) (result i32) (i32.atomic.rmw.xchg $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw8.xchg_2") (param i64 i32) (result i32) (i32.atomic.rmw8.xchg_u $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i32.atomic.rmw16.xchg_2") (param i64 i32) (result i32) (i32.atomic.rmw16.xchg_u $mem offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i64 atomic Xchg
    (func (export "i64.atomic.rmw.xchg") (param i64 i64) (result i64) (i64.atomic.rmw.xchg $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw8.xchg") (param i64 i64) (result i64) (i64.atomic.rmw8.xchg_u $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw16.xchg") (param i64 i64) (result i64) (i64.atomic.rmw16.xchg_u $mem offset=16 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw32.xchg") (param i64 i64) (result i64) (i64.atomic.rmw32.xchg_u $mem offset=16 (local.get 0) (local.get 1)))

    (func (export "i64.atomic.rmw.xchg_2") (param i64 i64) (result i64) (i64.atomic.rmw.xchg $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw8.xchg_2") (param i64 i64) (result i64) (i64.atomic.rmw8.xchg_u $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw16.xchg_2") (param i64 i64) (result i64) (i64.atomic.rmw16.xchg_u $mem offset=0x100000000 (local.get 0) (local.get 1)))
    (func (export "i64.atomic.rmw32.xchg_2") (param i64 i64) (result i64) (i64.atomic.rmw32.xchg_u $mem offset=0x100000000 (local.get 0) (local.get 1)))

    ;; i32 atomic Cmpxchg
    (func (export "i32.atomic.rmw.cmpxchg") (param i64 i32 i32) (result i32) (i32.atomic.rmw.cmpxchg $mem offset=16 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i32.atomic.rmw8.cmpxchg") (param i64 i32 i32) (result i32) (i32.atomic.rmw8.cmpxchg_u $mem offset=16 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i32.atomic.rmw16.cmpxchg") (param i64 i32 i32) (result i32) (i32.atomic.rmw16.cmpxchg_u $mem offset=16 (local.get 0) (local.get 1) (local.get 2)))

    (func (export "i32.atomic.rmw.cmpxchg_2") (param i64 i32 i32) (result i32) (i32.atomic.rmw.cmpxchg $mem offset=0x100000000 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i32.atomic.rmw8.cmpxchg_2") (param i64 i32 i32) (result i32) (i32.atomic.rmw8.cmpxchg_u $mem offset=0x100000000 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i32.atomic.rmw16.cmpxchg_2") (param i64 i32 i32) (result i32) (i32.atomic.rmw16.cmpxchg_u $mem offset=0x100000000 (local.get 0) (local.get 1) (local.get 2)))

    ;; i64 atomic Cmpxchg
    (func (export "i64.atomic.rmw.cmpxchg") (param i64 i64 i64) (result i64) (i64.atomic.rmw.cmpxchg $mem offset=16 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i64.atomic.rmw8.cmpxchg") (param i64 i64 i64) (result i64) (i64.atomic.rmw8.cmpxchg_u $mem offset=16 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i64.atomic.rmw16.cmpxchg") (param i64 i64 i64) (result i64) (i64.atomic.rmw16.cmpxchg_u $mem offset=16 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i64.atomic.rmw32.cmpxchg") (param i64 i64 i64) (result i64) (i64.atomic.rmw32.cmpxchg_u $mem offset=16 (local.get 0) (local.get 1) (local.get 2)))

    (func (export "i64.atomic.rmw.cmpxchg_2") (param i64 i64 i64) (result i64) (i64.atomic.rmw.cmpxchg $mem offset=0x100000000 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i64.atomic.rmw8.cmpxchg_2") (param i64 i64 i64) (result i64) (i64.atomic.rmw8.cmpxchg_u $mem offset=0x100000000 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i64.atomic.rmw16.cmpxchg_2") (param i64 i64 i64) (result i64) (i64.atomic.rmw16.cmpxchg_u $mem offset=0x100000000 (local.get 0) (local.get 1) (local.get 2)))
    (func (export "i64.atomic.rmw32.cmpxchg_2") (param i64 i64 i64) (result i64) (i64.atomic.rmw32.cmpxchg_u $mem offset=0x100000000 (local.get 0) (local.get 1) (local.get 2)))
)

;; i32
(assert_return (invoke "i32.store" (i64.const 0x10) (i32.const 1234)))
(assert_return (invoke "i32.load" (i64.const 0x10)) (i32.const 1234))
(assert_trap (invoke "i32.store" (i64.const 0x100000010) (i32.const 1234)) "out of bounds memory access")
(assert_trap (invoke "i32.load" (i64.const 0x100000010)) "out of bounds memory access")

(assert_return (invoke "i32.store_2" (i64.const 0x10) (i32.const 4321)))
(assert_return (invoke "i32.load_2" (i64.const 0x10)) (i32.const 4321))
(assert_trap (invoke "i32.store_2" (i64.const 0x100000010) (i32.const 4321)) "out of bounds memory access")
(assert_trap (invoke "i32.load_2" (i64.const 0x100000010)) "out of bounds memory access")

(assert_trap (invoke "i32.store_3" (i64.const 0x10) (i32.const 1234)) "out of bounds memory access")
(assert_trap (invoke "i32.load_3" (i64.const 0x10)) "out of bounds memory access")

;; i32_8
(assert_return (invoke "i32.store8" (i64.const 0x10) (i32.const 101)))
(assert_return (invoke "i32.load8u" (i64.const 0x10)) (i32.const 101))
(assert_return (invoke "i32.load8s" (i64.const 0x10)) (i32.const 101))
(assert_trap (invoke "i32.store8" (i64.const 0x100000000) (i32.const 101)) "out of bounds memory access")
(assert_trap (invoke "i32.load8u" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "i32.load8s" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i32.store8_2" (i64.const 0x10) (i32.const 101)) "out of bounds memory access")
(assert_trap (invoke "i32.load8u_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "i32.load8s_2" (i64.const 0x10)) "out of bounds memory access")

;; i32_16
(assert_return (invoke "i32.store16" (i64.const 0x10) (i32.const 32149)))
(assert_return (invoke "i32.load16u" (i64.const 0x10)) (i32.const 32149))
(assert_return (invoke "i32.load16s" (i64.const 0x10)) (i32.const 32149))
(assert_trap (invoke "i32.store16" (i64.const 0x100000000) (i32.const 32149)) "out of bounds memory access")
(assert_trap (invoke "i32.load16u" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "i32.load16s" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i32.store16_2" (i64.const 0x10) (i32.const 32149)) "out of bounds memory access")
(assert_trap (invoke "i32.load16u_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "i32.load16s_2" (i64.const 0x10)) "out of bounds memory access")

;; i64
(assert_return (invoke "i64.store" (i64.const 0x10) (i64.const 123456781234)))
(assert_return (invoke "i64.load" (i64.const 0x10)) (i64.const 123456781234))
(assert_trap (invoke "i64.store" (i64.const 0x100000010) (i64.const 123456781234)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const 0x100000010)) "out of bounds memory access")

(assert_return (invoke "i64.store_2" (i64.const 0x10) (i64.const 432187654321)))
(assert_return (invoke "i64.load_2" (i64.const 0x10)) (i64.const 432187654321))
(assert_trap (invoke "i64.store_2" (i64.const 0x100000010) (i64.const 432187654321)) "out of bounds memory access")
(assert_trap (invoke "i64.load_2" (i64.const 0x100000010)) "out of bounds memory access")

(assert_trap (invoke "i64.store_3" (i64.const 0x10) (i64.const 123456781234)) "out of bounds memory access")
(assert_trap (invoke "i64.load_3" (i64.const 0x10)) "out of bounds memory access")

;; i64_8
(assert_return (invoke "i64.store8" (i64.const 0x10) (i64.const 108)))
(assert_return (invoke "i64.load8u" (i64.const 0x10)) (i64.const 108))
(assert_return (invoke "i64.load8s" (i64.const 0x10)) (i64.const 108))
(assert_trap (invoke "i64.store8" (i64.const 0x100000000) (i64.const 108)) "out of bounds memory access")
(assert_trap (invoke "i64.load8u" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "i64.load8s" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i64.store8_2" (i64.const 0x10) (i64.const 108)) "out of bounds memory access")
(assert_trap (invoke "i64.load8u_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "i64.load8s_2" (i64.const 0x10)) "out of bounds memory access")

;; i64_16
(assert_return (invoke "i64.store16" (i64.const 0x10) (i64.const 30571)))
(assert_return (invoke "i64.load16u" (i64.const 0x10)) (i64.const 30571))
(assert_return (invoke "i64.load16s" (i64.const 0x10)) (i64.const 30571))
(assert_trap (invoke "i64.store16" (i64.const 0x100000000) (i64.const 30571)) "out of bounds memory access")
(assert_trap (invoke "i64.load16u" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "i64.load16s" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i64.store16_2" (i64.const 0x10) (i64.const 30571)) "out of bounds memory access")
(assert_trap (invoke "i64.load16u_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "i64.load16s_2" (i64.const 0x10)) "out of bounds memory access")

;; i64_32
(assert_return (invoke "i64.store32" (i64.const 0x10) (i64.const 2058934)))
(assert_return (invoke "i64.load32u" (i64.const 0x10)) (i64.const 2058934))
(assert_return (invoke "i64.load32s" (i64.const 0x10)) (i64.const 2058934))
(assert_trap (invoke "i64.store32" (i64.const 0x100000000) (i64.const 2058934)) "out of bounds memory access")
(assert_trap (invoke "i64.load32u" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "i64.load32s" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i64.store32_2" (i64.const 0x10) (i64.const 2058934)) "out of bounds memory access")
(assert_trap (invoke "i64.load32u_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "i64.load32s_2" (i64.const 0x10)) "out of bounds memory access")

;; f32
(assert_return (invoke "f32.store" (i64.const 0x10) (f32.const 1234.5)))
(assert_return (invoke "f32.load" (i64.const 0x10)) (f32.const 1234.5))
(assert_trap (invoke "f32.store" (i64.const 0x100000000) (f32.const 1234.5)) "out of bounds memory access")
(assert_trap (invoke "f32.load" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "f32.store_2" (i64.const 0x10) (f32.const 1234.5)) "out of bounds memory access")
(assert_trap (invoke "f32.load_2" (i64.const 0x10)) "out of bounds memory access")

;; f64
(assert_return (invoke "f64.store" (i64.const 0x10) (f64.const 123456789.5)))
(assert_return (invoke "f64.load" (i64.const 0x10)) (f64.const 123456789.5))
(assert_trap (invoke "f64.store" (i64.const 0x100000000) (f64.const 123456789.5)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "f64.store_2" (i64.const 0x10) (f64.const 123456789.5)) "out of bounds memory access")
(assert_trap (invoke "f64.load_2" (i64.const 0x10)) "out of bounds memory access")

;; v128
(assert_return (invoke "v128.store" (i64.const 0x10) (v128.const i64x2 1 2)))
(assert_return (invoke "v128.load" (i64.const 0x10)) (v128.const i64x2 1 2))
(assert_trap (invoke "v128.store" (i64.const 0x100000000) (v128.const i64x2 1 2)) "out of bounds memory access")
(assert_trap (invoke "v128.load" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.store_2" (i64.const 0x10) (v128.const i64x2 1 2)) "out of bounds memory access")
(assert_trap (invoke "v128.load_2" (i64.const 0x10)) "out of bounds memory access")

;; clear mem
(assert_return (invoke "v128.store" (i64.const 0x10) (v128.const i64x2 0 0)))

;; v128 8x8
(assert_return (invoke "v128.load8x8s" (i64.const 0x10)) (v128.const i64x2 0 0))
(assert_return (invoke "v128.load8x8u" (i64.const 0x10)) (v128.const i64x2 0 0))

(assert_trap (invoke "v128.load8x8s" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "v128.load8x8u" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.load8x8s_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "v128.load8x8u_2" (i64.const 0x10)) "out of bounds memory access")

;; v128 16x4
(assert_return (invoke "v128.load16x4s" (i64.const 0x10)) (v128.const i64x2 0 0))
(assert_return (invoke "v128.load16x4u" (i64.const 0x10)) (v128.const i64x2 0 0))

(assert_trap (invoke "v128.load16x4s" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "v128.load16x4u" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.load16x4s_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "v128.load16x4u_2" (i64.const 0x10)) "out of bounds memory access")

;; v128 32x2
(assert_return (invoke "v128.load32x2s" (i64.const 0x10)) (v128.const i64x2 0 0))
(assert_return (invoke "v128.load32x2u" (i64.const 0x10)) (v128.const i64x2 0 0))

(assert_trap (invoke "v128.load32x2s" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "v128.load32x2u" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.load32x2s_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "v128.load32x2u_2" (i64.const 0x10)) "out of bounds memory access")

;; v128 splat
(assert_return (invoke "v128.load8splat" (i64.const 0x10)) (v128.const i64x2 0 0))
(assert_return (invoke "v128.load16splat" (i64.const 0x10)) (v128.const i64x2 0 0))
(assert_return (invoke "v128.load32splat" (i64.const 0x10)) (v128.const i64x2 0 0))
(assert_return (invoke "v128.load64splat" (i64.const 0x10)) (v128.const i64x2 0 0))

(assert_trap (invoke "v128.load8splat" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "v128.load16splat" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "v128.load32splat" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "v128.load64splat" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.load8splat_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "v128.load16splat_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "v128.load32splat_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "v128.load64splat_2" (i64.const 0x10)) "out of bounds memory access")

;; v128 lane 8
(assert_return (invoke "v128.store8lane" (i64.const 0x10) (v128.const i8x16 75 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)))
(assert_return (invoke "v128.load8lane" (i64.const 0x10)) (v128.const i8x16 75 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0))

(assert_trap (invoke "v128.store8lane" (i64.const 0x100000000) (v128.const i64x2 0 0)) "out of bounds memory access")
(assert_trap (invoke "v128.load8lane" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.store8lane_2" (i64.const 0x10) (v128.const i64x2 0 0)) "out of bounds memory access")
(assert_trap (invoke "v128.load8lane_2" (i64.const 0x10)) "out of bounds memory access")

;; v128 lane 16
(assert_return (invoke "v128.store16lane" (i64.const 0x10) (v128.const i16x8 30678 0 0 0 0 0 0 0)))
(assert_return (invoke "v128.load16lane" (i64.const 0x10)) (v128.const i16x8 30678 0 0 0 0 0 0 0))

(assert_trap (invoke "v128.store16lane" (i64.const 0x100000000) (v128.const i64x2 0 0)) "out of bounds memory access")
(assert_trap (invoke "v128.load16lane" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.store16lane_2" (i64.const 0x10) (v128.const i64x2 0 0)) "out of bounds memory access")
(assert_trap (invoke "v128.load16lane_2" (i64.const 0x10)) "out of bounds memory access")

;; v128 lane 32
(assert_return (invoke "v128.store32lane" (i64.const 0x10) (v128.const i32x4 2180785617 0 0 0)))
(assert_return (invoke "v128.load32lane" (i64.const 0x10)) (v128.const i32x4 2180785617 0 0 0))

(assert_trap (invoke "v128.store32lane" (i64.const 0x100000000) (v128.const i64x2 0 0)) "out of bounds memory access")
(assert_trap (invoke "v128.load32lane" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.store32lane_2" (i64.const 0x10) (v128.const i64x2 0 0)) "out of bounds memory access")
(assert_trap (invoke "v128.load32lane_2" (i64.const 0x10)) "out of bounds memory access")

;; v128 lane 64
(assert_return (invoke "v128.store64lane" (i64.const 0x10) (v128.const i64x2 314756237046123789 0)))
(assert_return (invoke "v128.load64lane" (i64.const 0x10)) (v128.const i64x2 314756237046123789 0))

(assert_trap (invoke "v128.store64lane" (i64.const 0x100000000) (v128.const i64x2 0 0)) "out of bounds memory access")
(assert_trap (invoke "v128.load64lane" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.store64lane_2" (i64.const 0x10) (v128.const i64x2 0 0)) "out of bounds memory access")
(assert_trap (invoke "v128.load64lane_2" (i64.const 0x10)) "out of bounds memory access")

;; v128 load zero
(assert_return (invoke "i32.store_2" (i64.const 0x10) (i32.const 1579840812)))
(assert_return (invoke "v128.load32zero" (i64.const 0x10)) (v128.const i32x4 1579840812 0 0 0))
(assert_return (invoke "i64.store_2" (i64.const 0x10) (i64.const 84592345257645125)))
(assert_return (invoke "v128.load64zero" (i64.const 0x10)) (v128.const i64x2 84592345257645125 0))

(assert_trap (invoke "v128.load32zero" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "v128.load64zero" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "v128.load32zero_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "v128.load64zero_2" (i64.const 0x10)) "out of bounds memory access")

;; atomic core
(assert_return (invoke "atomic.notify" (i64.const 0x10)))
(assert_return (invoke "memory.atomic.wait32" (i64.const 0x10)))
(assert_return (invoke "memory.atomic.wait64" (i64.const 0x10)))

(assert_trap (invoke "atomic.notify" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "memory.atomic.wait32" (i64.const 0x100000000)) "out of bounds memory access")
(assert_trap (invoke "memory.atomic.wait64" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "atomic.notify_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "memory.atomic.wait32_2" (i64.const 0x10)) "out of bounds memory access")
(assert_trap (invoke "memory.atomic.wait64_2" (i64.const 0x10)) "out of bounds memory access")

;; i32 atomic
(assert_return (invoke "i32.atomic.store" (i64.const 0x10) (i32.const 1480532187)))
(assert_return (invoke "i32.atomic.load" (i64.const 0x10)) (i32.const 1480532187))

(assert_trap (invoke "i32.atomic.store" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.load" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i32.atomic.store_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.load_2" (i64.const 0x10)) "out of bounds memory access")

;; i32 atomic 8
(assert_return (invoke "i32.atomic.store8" (i64.const 0x10) (i32.const 245)))
(assert_return (invoke "i32.atomic.load8" (i64.const 0x10)) (i32.const 245))

(assert_trap (invoke "i32.atomic.store8" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.load8" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i32.atomic.store8_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.load8_2" (i64.const 0x10)) "out of bounds memory access")

;; i32 atomic 16
(assert_return (invoke "i32.atomic.store16" (i64.const 0x10) (i32.const 63416)))
(assert_return (invoke "i32.atomic.load16" (i64.const 0x10)) (i32.const 63416))

(assert_trap (invoke "i32.atomic.store16" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.load16" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i32.atomic.store16_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.load16_2" (i64.const 0x10)) "out of bounds memory access")

;; i64 atomic
(assert_return (invoke "i64.atomic.store" (i64.const 0x10) (i64.const 8923601264510235)))
(assert_return (invoke "i64.atomic.load" (i64.const 0x10)) (i64.const 8923601264510235))

(assert_trap (invoke "i64.atomic.store" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.load" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.store_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.load_2" (i64.const 0x10)) "out of bounds memory access")

;; i64 atomic 8
(assert_return (invoke "i64.atomic.store8" (i64.const 0x10) (i64.const 208)))
(assert_return (invoke "i64.atomic.load8" (i64.const 0x10)) (i64.const 208))

(assert_trap (invoke "i64.atomic.store8" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.load8" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.store8_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.load8_2" (i64.const 0x10)) "out of bounds memory access")

;; i64 atomic 16
(assert_return (invoke "i64.atomic.store16" (i64.const 0x10) (i64.const 58930)))
(assert_return (invoke "i64.atomic.load16" (i64.const 0x10)) (i64.const 58930))

(assert_trap (invoke "i64.atomic.store16" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.load16" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.store16_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.load16_2" (i64.const 0x10)) "out of bounds memory access")

;; i64 atomic 32
(assert_return (invoke "i64.atomic.store32" (i64.const 0x10) (i64.const 4035164968)))
(assert_return (invoke "i64.atomic.load32" (i64.const 0x10)) (i64.const 4035164968))

(assert_trap (invoke "i64.atomic.store32" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.load32" (i64.const 0x100000000)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.store32_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.load32_2" (i64.const 0x10)) "out of bounds memory access")

;; i32 atomic rmw
(assert_return (invoke "i32.store_2" (i64.const 0x10) (i32.const 0)))
(assert_return (invoke "i32.atomic.rmw.add" (i64.const 0x10) (i32.const 1684902794)) (i32.const 0))
(assert_return (invoke "i32.atomic.rmw.sub" (i64.const 0x10) (i32.const 1684902794)) (i32.const 1684902794))

(assert_trap (invoke "i32.atomic.rmw.add" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw.sub" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")

(assert_trap (invoke "i32.atomic.rmw.add_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw.sub_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")

;; i32 atomic rmw 8
(assert_return (invoke "i32.store_2" (i64.const 0x10) (i32.const 0)))
(assert_return (invoke "i32.atomic.rmw8.or" (i64.const 0x10) (i32.const 159)) (i32.const 0))
(assert_return (invoke "i32.atomic.rmw8.xor" (i64.const 0x10) (i32.const 159)) (i32.const 159))

(assert_trap (invoke "i32.atomic.rmw8.or" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw8.xor" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")

(assert_trap (invoke "i32.atomic.rmw8.or_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw8.xor_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")

;; i32 atomic rmw 16
(assert_return (invoke "i32.store_2" (i64.const 0x10) (i32.const 0)))
(assert_return (invoke "i32.atomic.rmw16.or" (i64.const 0x10) (i32.const 159)) (i32.const 0))
(assert_return (invoke "i32.atomic.rmw16.and" (i64.const 0x10) (i32.const 159)) (i32.const 159))

(assert_trap (invoke "i32.atomic.rmw16.or" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw16.and" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")

(assert_trap (invoke "i32.atomic.rmw16.or_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw16.and_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")

;; i64 atomic rmw
(assert_return (invoke "i64.store_2" (i64.const 0x10) (i64.const 0)))
(assert_return (invoke "i64.atomic.rmw.add" (i64.const 0x10) (i64.const 194602756123704341)) (i64.const 0))
(assert_return (invoke "i64.atomic.rmw.sub" (i64.const 0x10) (i64.const 194602756123704341)) (i64.const 194602756123704341))

(assert_trap (invoke "i64.atomic.rmw.add" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw.sub" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.rmw.add_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw.sub_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")

;; i64 atomic rmw 8
(assert_return (invoke "i64.store_2" (i64.const 0x10) (i64.const 0)))
(assert_return (invoke "i64.atomic.rmw8.or" (i64.const 0x10) (i64.const 180)) (i64.const 0))
(assert_return (invoke "i64.atomic.rmw8.xor" (i64.const 0x10) (i64.const 180)) (i64.const 180))

(assert_trap (invoke "i64.atomic.rmw8.or" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw8.xor" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.rmw8.or_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw8.xor_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")

;; i64 atomic rmw 16
(assert_return (invoke "i64.store_2" (i64.const 0x10) (i64.const 0)))
(assert_return (invoke "i64.atomic.rmw16.or" (i64.const 0x10) (i64.const 59468)) (i64.const 0))
(assert_return (invoke "i64.atomic.rmw16.and" (i64.const 0x10) (i64.const 59468)) (i64.const 59468))

(assert_trap (invoke "i64.atomic.rmw16.or" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw16.and" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.rmw16.or_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw16.and_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")

;; i64 atomic rmw 32
(assert_return (invoke "i64.store_2" (i64.const 0x10) (i64.const 0)))
(assert_return (invoke "i64.atomic.rmw32.add" (i64.const 0x10) (i64.const 1962460524)) (i64.const 0))
(assert_return (invoke "i64.atomic.rmw32.and" (i64.const 0x10) (i64.const 1962460524)) (i64.const 1962460524))

(assert_trap (invoke "i64.atomic.rmw32.add" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw32.and" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.rmw32.add_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw32.and_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")

;; i32 atomic Xchg
(assert_return (invoke "i32.store_2" (i64.const 0x10) (i32.const 1683920478)))
(assert_return (invoke "i32.atomic.rmw.xchg" (i64.const 0x10) (i32.const 0)) (i32.const 1683920478))
(assert_return (invoke "i32.store8" (i64.const 0x10) (i32.const 238)))
(assert_return (invoke "i32.atomic.rmw8.xchg" (i64.const 0x10) (i32.const 0)) (i32.const 238))
(assert_return (invoke "i32.store16" (i64.const 0x10) (i32.const 57840)))
(assert_return (invoke "i32.atomic.rmw16.xchg" (i64.const 0x10) (i32.const 0)) (i32.const 57840))

(assert_trap (invoke "i32.atomic.rmw.xchg" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw8.xchg" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw16.xchg" (i64.const 0x100000000) (i32.const 0)) "out of bounds memory access")

(assert_trap (invoke "i32.atomic.rmw.xchg_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw8.xchg_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw16.xchg_2" (i64.const 0x10) (i32.const 0)) "out of bounds memory access")

;; i64 atomic Xchg
(assert_return (invoke "i64.store_2" (i64.const 0x10) (i64.const 7686420451258)))
(assert_return (invoke "i64.atomic.rmw.xchg" (i64.const 0x10) (i64.const 0)) (i64.const 7686420451258))
(assert_return (invoke "i64.store8" (i64.const 0x10) (i64.const 226)))
(assert_return (invoke "i64.atomic.rmw8.xchg" (i64.const 0x10) (i64.const 0)) (i64.const 226))
(assert_return (invoke "i64.store16" (i64.const 0x10) (i64.const 53651)))
(assert_return (invoke "i64.atomic.rmw16.xchg" (i64.const 0x10) (i64.const 0)) (i64.const 53651))
(assert_return (invoke "i64.store32" (i64.const 0x10) (i64.const 1687254093)))
(assert_return (invoke "i64.atomic.rmw32.xchg" (i64.const 0x10) (i64.const 0)) (i64.const 1687254093))

(assert_trap (invoke "i64.atomic.rmw.xchg" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw8.xchg" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw16.xchg" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw32.xchg" (i64.const 0x100000000) (i64.const 0)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.rmw.xchg_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw8.xchg_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw16.xchg_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw32.xchg_2" (i64.const 0x10) (i64.const 0)) "out of bounds memory access")

;; i32 atomic Cmpxchg
(assert_return (invoke "i32.store_2" (i64.const 0x10) (i32.const 1809274815)))
(assert_return (invoke "i32.atomic.rmw.cmpxchg" (i64.const 0x10) (i32.const 1809274815) (i32.const 0)) (i32.const 1809274815))
(assert_return (invoke "i32.store8" (i64.const 0x10) (i32.const 164)))
(assert_return (invoke "i32.atomic.rmw8.cmpxchg" (i64.const 0x10) (i32.const 164) (i32.const 0)) (i32.const 164))
(assert_return (invoke "i32.store16" (i64.const 0x10) (i32.const 59218)))
(assert_return (invoke "i32.atomic.rmw16.cmpxchg" (i64.const 0x10) (i32.const 59218) (i32.const 0)) (i32.const 59218))

(assert_trap (invoke "i32.atomic.rmw.cmpxchg" (i64.const 0x100000000) (i32.const 0) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw8.cmpxchg" (i64.const 0x100000000) (i32.const 0) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw16.cmpxchg" (i64.const 0x100000000) (i32.const 0) (i32.const 0)) "out of bounds memory access")

(assert_trap (invoke "i32.atomic.rmw.cmpxchg_2" (i64.const 0x10) (i32.const 0) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw8.cmpxchg_2" (i64.const 0x10) (i32.const 0) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.atomic.rmw16.cmpxchg_2" (i64.const 0x10) (i32.const 0) (i32.const 0)) "out of bounds memory access")

;; i64 atomic Cmpxchg
(assert_return (invoke "i64.store_2" (i64.const 0x10) (i64.const 127857628907652)))
(assert_return (invoke "i64.atomic.rmw.cmpxchg" (i64.const 0x10) (i64.const 127857628907652) (i64.const 0)) (i64.const 127857628907652))
(assert_return (invoke "i64.store8" (i64.const 0x10) (i64.const 201)))
(assert_return (invoke "i64.atomic.rmw8.cmpxchg" (i64.const 0x10) (i64.const 201) (i64.const 0)) (i64.const 201))
(assert_return (invoke "i64.store16" (i64.const 0x10) (i64.const 60437)))
(assert_return (invoke "i64.atomic.rmw16.cmpxchg" (i64.const 0x10) (i64.const 60437) (i64.const 0)) (i64.const 60437))
(assert_return (invoke "i64.store32" (i64.const 0x10) (i64.const 2067381469)))
(assert_return (invoke "i64.atomic.rmw32.cmpxchg" (i64.const 0x10) (i64.const 2067381469) (i64.const 0)) (i64.const 2067381469))

(assert_trap (invoke "i64.atomic.rmw.cmpxchg" (i64.const 0x100000000) (i64.const 0) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw8.cmpxchg" (i64.const 0x100000000) (i64.const 0) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw16.cmpxchg" (i64.const 0x100000000) (i64.const 0) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw32.cmpxchg" (i64.const 0x100000000) (i64.const 0) (i64.const 0)) "out of bounds memory access")

(assert_trap (invoke "i64.atomic.rmw.cmpxchg_2" (i64.const 0x10) (i64.const 0) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw8.cmpxchg_2" (i64.const 0x10) (i64.const 0) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw16.cmpxchg_2" (i64.const 0x10) (i64.const 0) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.atomic.rmw32.cmpxchg_2" (i64.const 0x10) (i64.const 0) (i64.const 0)) "out of bounds memory access")
