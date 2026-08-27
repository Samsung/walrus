(module
    (import "wasi_snapshot_preview1" "sched_yield" (func $sched_yield (result i32)))
	(func (export "sched_yield") (result i32)
		call $sched_yield
	)
)

(assert_return (invoke "sched_yield") (i32.const 0))
