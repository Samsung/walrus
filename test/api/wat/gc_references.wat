;; PR #536: opaque host references must survive Wasm GC containers and calls.
(module
  (import "host" "collect" (func $collect))
  (type $box (struct (field externref)))
  (type $items (array (mut externref)))
  (type $outer (struct (field (ref $items))))
  (global $g (mut (ref null $box)) (ref.null $box))

  ;; The reviewer's global -> struct -> externref example.
  (func (export "set") (param externref)
    local.get 0
    struct.new $box
    global.set $g)
  (func $get (export "get") (result externref)
    global.get $g
    struct.get $box 0)
  (func $clear (export "clear")
    ref.null $box
    global.set $g)

  ;; The external object is only in a Wasm local across a reentrant host call.
  (func (export "take") (result externref) (local externref)
    call $get
    local.set 0
    call $clear
    call $collect
    local.get 0)

  ;; Return a GC root containing an external object and a self-referencing array.
  (func (export "make") (param externref) (result externref)
    (local (ref $items))
    local.get 0
    ref.null extern
    array.new_fixed $items 2
    local.set 1
    local.get 1
    i32.const 1
    local.get 1
    extern.convert_any
    array.set $items
    local.get 1
    struct.new $outer
    extern.convert_any)
  (func (export "read") (param externref) (result externref)
    local.get 0
    any.convert_extern
    ref.cast (ref $outer)
    struct.get $outer 0
    i32.const 0
    array.get $items)
  (func (export "replace") (param externref externref)
    local.get 0
    any.convert_extern
    ref.cast (ref $outer)
    struct.get $outer 0
    i32.const 0
    local.get 1
    array.set $items)

  ;; Only the box is live; tracing must cross the GC heap to find its externref.
  (func (export "take-box") (result externref) (local (ref null $box))
    global.get $g
    local.set 0
    call $clear
    call $collect
    local.get 0
    struct.get $box 0)

  ;; The reference is stored after more than 2 KiB of locals.
  (func $large (export "take-large") (result externref)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local $kept externref)
    call $get
    local.set $kept
    call $clear
    call $collect
    local.get $kept)

  ;; Interpreter tail calls must register a buffer grown from a small frame.
  (func (export "take-tail") (result externref)
    return_call $large)

  (func (export "take-trap") (local externref)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    call $get
    local.set 0
    call $clear
    call $collect
    local.get 0
    drop
    unreachable)
)
