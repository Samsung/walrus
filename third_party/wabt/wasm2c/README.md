# wasm2c: Convert wasm files to C source and header

`wasm2c` takes a WebAssembly module and produces an equivalent C source and
header. Some examples:

```sh
# parse binary file test.wasm and write test.c and test.h
$ wasm2c test.wasm -o test.c

# parse test.wasm, write test.c and test.h, but ignore the debug names, if any
$ wasm2c test.wasm --no-debug-names -o test.c
```

## Tutorial: .wat -> .wasm -> .c

Let's look at a simple example of a factorial function.

```wasm
(memory $mem 1)
(func (export "fac") (param $x i32) (result i32)
  (if (result i32) (i32.eq (local.get $x) (i32.const 0))
    (then (i32.const 1))
    (else
      (i32.mul (local.get $x) (call 0 (i32.sub (local.get $x) (i32.const 1))))
    )
  )
)
```

Save this to `fac.wat`. We can convert this to a `.wasm` file by using the
`wat2wasm` tool:

```sh
$ wat2wasm fac.wat -o fac.wasm
```

We can then convert it to a C source and header by using the `wasm2c` tool:

```sh
$ wasm2c fac.wasm -o fac.c
```

This generates two files, `fac.c` and `fac.h`. We'll take a closer look at
these files below, but first let's show a simple example of how to use these
files.

## Using the generated module

To actually use our `fac` module, we'll use create a new file, `main.c`, that
include `fac.h`, initializes the module, and calls `fac`.

`wasm2c` generates a few C symbols based on the `fac.wasm` module: `Z_fac_init_module`, `Z_fac_instantiate`
and `Z_facZ_fac`.  The first initializes the module, the second constructs an instance of the module, and the third is the
exported `fac` function.

All the exported symbols shared a common prefix (`Z_fac`) which, by default, is
based on the name section in the module or the name of input file.  This prefix
can be overridden using the `-n/--module-name` command line flag.

In addition to parameters defined in `fac.wat`, `Z_fac_instantiate` and `Z_facZ_fac`
take in a pointer to a `Z_fac_instance_t`.  The structure is used to
store the context information of the module instance, and `main.c` is responsible
for providing it.

```c
#include <stdio.h>
#include <stdlib.h>

#include "fac.h"

int main(int argc, char** argv) {
  /* Make sure there is at least one command-line argument. */
  if (argc < 2)
    return 1;

  /* Convert the argument from a string to an int. We'll implicitly cast the int
  to a `u32`, which is what `fac` expects. */
  u32 x = atoi(argv[1]);

  /* Initialize the Wasm runtime. */
  wasm_rt_init();

  /* Initialize the `fac` module (this registers the module's function types in
   * a global data structure) */
  Z_fac_init_module();

  /* Declare an instance of the `fac` module. */
  Z_fac_instance_t instance;

  /* Construct the module instance. */
  Z_fac_instantiate(&instance);

  /* Call `fac`, using the mangled name. */
  u32 result = Z_facZ_fac(&instance, x);

  /* Print the result. */
  printf("fac(%u) -> %u\n", x, result);

  /* Free the fac module. */
  Z_fac_free(&instance);

  /* Free the Wasm runtime state. */
  wasm_rt_free();

  return 0;
}

```

## Compiling the wasm2c output

To compile the executable, we need to use `main.c` and the generated `fac.c`.
We'll also include `wasm-rt-impl.c` which has implementations of the various
`wasm_rt_*` functions used by `fac.c` and `fac.h`.

```sh
$ cc -o fac main.c fac.c wasm-rt-impl.c
```

A note on compiling with optimization: wasm2c relies on certain
behavior from the C compiler to maintain conformance with the
WebAssembly specification, especially with regards to requirements to
convert "signaling" to "quiet" floating-point NaN values and for
infinite recursion to produce a trap. When compiling with optimization
(e.g. `-O2` or `-O3`), it's necessary to disable some optimizations to
preserve conformance. With GCC 11, adding the command-line arguments
`-fno-optimize-sibling-calls -frounding-math -fsignaling-nans` appears
to be sufficient. With clang 14, just `-fno-optimize-sibling-calls
-frounding-math` appears to be sufficient.

Now let's test it out!

```sh
$ ./fac 1
fac(1) -> 1
$ ./fac 5
fac(5) -> 120
$ ./fac 10
fac(10) -> 3628800
```

You can take a look at the all of these files in
[wasm2c/examples/fac](/wasm2c/examples/fac).

## Looking at the generated header, `fac.h`

The generated header file looks something like this:

```c
#ifndef FAC_H_GENERATED_
#define FAC_H_GENERATED_

...

#include "wasm-rt.h"

...
#ifndef WASM_RT_CORE_TYPES_DEFINED
#define WASM_RT_CORE_TYPES_DEFINED

...

#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Z_fac_instance_t {
  char dummy_member;
} Z_fac_instance_t;

void Z_fac_init_module(void);
void Z_fac_instantiate(Z_fac_instance_t*);
void Z_fac_free(Z_fac_instance_t*);

/* export: 'fac' */
u32 Z_facZ_fac(Z_fac_instance_t*, u32);

#ifdef __cplusplus
}
#endif

#endif  /* FAC_H_GENERATED_ */

```

Let's look at each section. The outer `#ifndef` is standard C
boilerplate for a header. This `WASM_RT_CORE_TYPES_DEFINED` section
contains a number of definitions required for all WebAssembly
modules. The `extern "C"` part makes sure to not mangle the symbols if
using this header in C++.

The included `wasm-rt.h` file also includes a number of relevant definitions.
First is the `wasm_rt_trap_t` enum, which is used to give the reason a trap
occurred.

```c
typedef enum {
  WASM_RT_TRAP_NONE,
  WASM_RT_TRAP_OOB,
  WASM_RT_TRAP_INT_OVERFLOW,
  WASM_RT_TRAP_DIV_BY_ZERO,
  WASM_RT_TRAP_INVALID_CONVERSION,
  WASM_RT_TRAP_UNREACHABLE,
  WASM_RT_TRAP_CALL_INDIRECT,
  WASM_RT_TRAP_UNCAUGHT_EXCEPTION,
  WASM_RT_TRAP_EXHAUSTION,
} wasm_rt_trap_t;
```

Next is the `wasm_rt_type_t` enum, which is used for specifying function
signatures. The four WebAssembly value types are included:

```c
typedef enum {
  WASM_RT_I32,
  WASM_RT_I64,
  WASM_RT_F32,
  WASM_RT_F64,
} wasm_rt_type_t;
```

Next is `wasm_rt_funcref_t`, the function signature for a generic function
callback. Since a WebAssembly table can contain functions of any given
signature, it is necessary to convert them to a canonical form:

```c
typedef void (*wasm_rt_funcref_t)(void);
```

Next are the definitions for a table element. `func_type` is a function index
as returned by `wasm_rt_register_func_type` described below. `module_instance`
is the pointer to the module instance that should be passed in when the func is
called.

```c
typedef struct {
  uint32_t func_type;
  wasm_rt_funcref_t func;
  void* module_instance;
} wasm_rt_elem_t;
```

Next is the definition of a memory instance. The `data` field is a pointer to
`size` bytes of linear memory. The `size` field of `wasm_rt_memory_t` is the
current size of the memory instance in bytes, whereas `pages` is the current
size in pages (65536 bytes.) `max_pages` is the maximum number of pages as
specified by the module, or `0xffffffff` if there is no limit.

```c
typedef struct {
  uint8_t* data;
  uint32_t pages, max_pages;
  uint32_t size;
} wasm_rt_memory_t;
```

Next is the definition of a table instance. The `data` field is a pointer to
`size` elements. Like a memory instance, `size` is the current size of a table,
and `max_size` is the maximum size of the table, or `0xffffffff` if there is no
limit.

```c
typedef struct {
  wasm_rt_elem_t* data;
  uint32_t max_size;
  uint32_t size;
} wasm_rt_table_t;
```

## Symbols that must be defined by the embedder

Next in `wasm-rt.h` are a collection of function declarations that must be implemented by
the embedder (i.e. you) before this C source can be used.

A C implementation of these functions is defined in
[`wasm-rt-impl.h`](wasm-rt-impl.h) and [`wasm-rt-impl.c`](wasm-rt-impl.c).

```c
void wasm_rt_init(void);
bool wasm_rt_is_initialized(void);
void wasm_rt_free(void);
void wasm_rt_trap(wasm_rt_trap_t) __attribute__((noreturn));
const char* wasm_rt_strerror(wasm_rt_trap_t trap);
uint32_t wasm_rt_register_func_type(uint32_t params, uint32_t results, ...);
void wasm_rt_allocate_memory(wasm_rt_memory_t*, uint32_t initial_pages, uint32_t max_pages);
uint32_t wasm_rt_grow_memory(wasm_rt_memory_t*, uint32_t pages);
void wasm_rt_free_memory(wasm_rt_memory_t*);
void wasm_rt_allocate_table(wasm_rt_table_t*, uint32_t elements, uint32_t max_elements);
void wasm_rt_free_table(wasm_rt_table_t*);
uint32_t wasm_rt_call_stack_depth; /* on platforms that don't use the signal handler to detect exhaustion */
```

`wasm_rt_init` must be called by the embedder before anything else, to
initialize the runtime. `wasm_rt_free` frees any global
state. `wasm_rt_is_initialized` can be used to confirm that the
runtime has been initialized.

`wasm_rt_trap` is a function that is called when the module traps. Some
possible implementations are to throw a C++ exception, or to just abort the
program execution.

`wasm_rt_register_func_type` is a function that registers a function type. It
is a variadic function where the first two arguments give the number of
parameters and results, and the following arguments are the types. For example,
the function `func (param i32 f32) (result f64)` would register the function
type as
`wasm_rt_register_func_type(2, 1, WASM_RT_I32, WASM_RT_F32, WASM_RT_F64)`.

`wasm_rt_allocate_memory` initializes a memory instance, and allocates at least
enough space for the given number of initial pages. The memory must be cleared
to zero.

`wasm_rt_grow_memory` must grow the given memory instance by the given number
of pages. If there isn't enough memory to do so, or the new page count would be
greater than the maximum page count, the function must fail by returning
`0xffffffff`. If the function succeeds, it must return the previous size of the
memory instance, in pages.

`wasm_rt_free_memory` frees the memory instance.

`wasm_rt_allocate_table` initializes a table instance, and allocates at least
enough space for the given number of initial elements. The elements must be
cleared to zero.

`wasm_rt_free_table` frees the table instance.

`wasm_rt_call_stack_depth` is the current stack call depth. Since this is
shared between modules, it must be defined only once, by the embedder.
It is only used on platforms that don't use the signal handler to detect
exhaustion.

### Runtime support for exception handling

Several additional symbols must be defined if wasm2c is being run with
support for exceptions (`--enable-exceptions`):

```c
uint32_t wasm_rt_register_tag(uint32_t size);
void wasm_rt_load_exception(uint32_t tag, uint32_t size, const void* values);
WASM_RT_NO_RETURN void wasm_rt_throw(void);
WASM_RT_UNWIND_TARGET
WASM_RT_UNWIND_TARGET* wasm_rt_get_unwind_target(void);
void wasm_rt_set_unwind_target(WASM_RT_UNWIND_TARGET* target);
uint32_t wasm_rt_exception_tag(void);
uint32_t wasm_rt_exception_size(void);
void* wasm_rt_exception(void);
wasm_rt_try(target)
```

A C implementation of these functions is also available in
[`wasm-rt-impl.h`](wasm-rt-impl.h) and [`wasm-rt-impl.c`](wasm-rt-impl.c).

`wasm_rt_register_tag` registers an exception type (a tag) with a given size.

`wasm_rt_load_exception` sets the active exception to a given tag, size, and contents.

`wasm_rt_throw` throws the active exception.

`WASM_RT_UNWIND_TARGET` is the type of an unwind target if an
exception is thrown and caught.

`wasm_rt_get_unwind_target` gets the current unwind target if an exception is thrown.

`wasm_rt_set_unwind_target` sets the unwind target if an exception is thrown.

Three functions provide access to the active exception:
`wasm_rt_exception_tag`, `wasm_rt_exception_size`, and
`wasm_rt_exception` return its tag, size, and contents, respectively.

`wasm_rt_try(target)` is a macro that captures the current calling
environment as an unwind target and stores it into `target`, which
must be of type `WASM_RT_UNWIND_TARGET`.

## Exported symbols

Finally, `fac.h` defines the module instance type (which in the case
of `fac` is essentially empty), and the exported symbols provided by
the module. In our example, the only function we exported was
`fac`. `Z_fac_init_module()` initializes the whole module and must be
called before any instance of the module is used.

`Z_fac_instantiate(Z_fac_instance_t*)` creates an instance of
the module and must be called before the module instance can be
used. `Z_fac_free(Z_fac_instance_t*)` frees the instance.

```c
typedef struct Z_fac_instance_t {
  char dummy_member;
} Z_fac_instance_t;

void Z_fac_init_module(void);
void Z_fac_instantiate(Z_fac_instance_t*);
void Z_fac_free(Z_fac_instance_t*);

/* export: 'fac' */
u32 Z_facZ_fac(Z_fac_instance_t*, u32);
```

## Handling other kinds of imports and exports of modules

Exported functions are handled by declaring a prefixed equivalent
function in the header. If a module is imports a function, `wasm2c`
declares the function in the output header file, and the host function
is responsible for defining the function.

Exports of other kinds (globals, memories, tables) are handled
differently, since they are part of the module instance, and each
instance can have its own exports. For these cases, `wasm2c` provides
a function that takes in a module instance as argument, and returns
the corresponding export. For example, if `fac` exported a memory as
such:

```wasm
(export "mem" (memory $mem))
```

then `wasm2c` would declare the following function in the header:

```c
/* export: 'mem' */
extern wasm_rt_memory_t* Z_facZ_mem(Z_fac_instance_t*);
```

which would be defined as:
```c
/* export: 'mem' */
wasm_rt_memory_t* Z_fac_Z_mem(Z_fac_instance_t* instance) {
  return &instance->w2c_M0;
}
```

## A quick look at `fac.c`

The contents of `fac.c` are internals, but it is useful to see a little about
how it works.

The first few hundred lines define macros that are used to implement the
various WebAssembly instructions. Their implementations may be interesting to
the curious reader, but are out of scope for this document.

Following those definitions are various initialization functions (`init`, `free`,
`init_func_types`, `init_globals`, `init_memory`, `init_table`, and
`init_exports`.) In our example, most of these functions are empty, since the
module doesn't use any globals, memory or tables.

The most interesting part is the definition of the function `fac`:

```c
static u32 w2c_fac(Z_fac_instance_t* instance, u32 w2c_p0) {
  FUNC_PROLOGUE;
  u32 w2c_i0, w2c_i1, w2c_i2;
  w2c_i0 = w2c_p0;
  w2c_i1 = 0u;
  w2c_i0 = w2c_i0 == w2c_i1;
  if (w2c_i0) {
    w2c_i0 = 1u;
  } else {
    w2c_i0 = w2c_p0;
    w2c_i1 = w2c_p0;
    w2c_i2 = 1u;
    w2c_i1 -= w2c_i2;
    w2c_i1 = w2c_fac(instance, w2c_i1);
    w2c_i0 *= w2c_i1;
  }
  FUNC_EPILOGUE;
  return w2c_i0;
}
```

If you look at the original WebAssembly text in the flat format, you can see
that there is a 1-1 mapping in the output:

```wasm
(func $fac (param $x i32) (result i32)
  local.get $x
  i32.const 0
  i32.eq
  if (result i32)
    i32.const 1
  else
    local.get $x
    local.get $x
    i32.const 1
    i32.sub
    call 0
    i32.mul
  end)
```

This looks different than the factorial function above because it is using the
"flat format" instead of the "folded format". You can use `wat-desugar` to
convert between the two to be sure:

```sh
$ wat-desugar fac-flat.wat --fold -o fac-folded.wat
```

```wasm
(module
  (func (;0;) (param i32) (result i32)
    (if (result i32)  ;; label = @1
      (i32.eq
        (local.get 0)
        (i32.const 0))
      (then
        (i32.const 1))
      (else
        (i32.mul
          (local.get 0)
          (call 0
            (i32.sub
              (local.get 0)
              (i32.const 1)))))))
  (export "fac" (func 0))
  (type (;0;) (func (param i32) (result i32))))
```

The formatting is different and the variable and function names are gone, but
the structure is the same.

## Create multiple instances of a module

Since information about the execution context, such as memories, is encapsulated
in the module instance structure, and a pointer to the structure is being passed through 
function calls, multiple instances of the same module can be instantiated alongside
one another.

We can take a look at another version of the `main` function for a `rot13` example. By 
declaring two sets of context information, two instances of `rot13` can be instantiated 
in the same address space.

```c
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "rot13.h"

/* Define structure to hold the imports */
struct Z_host_instance_t {
  wasm_rt_memory_t memory;
  char* input;
};

/* Accessor to access the memory member of the host */
wasm_rt_memory_t* Z_hostZ_mem(struct Z_host_instance_t* instance) {
  return &instance->memory;
}

/* Declare the implementations of the imports. */
static u32 fill_buf(struct Z_host_instance_t* instance, u32 ptr, u32 size);
static void buf_done(struct Z_host_instance_t* instance, u32 ptr, u32 size);

/* Define host-provided functions under the names imported by the `rot13` instance */
u32 Z_hostZ_fill_buf(struct Z_host_instance_t* instance,
                     u32 ptr,
                     u32 size) {
  return fill_buf(instance, ptr, size);
}

void Z_hostZ_buf_done(struct Z_host_instance_t* instance,
                      u32 ptr,
                      u32 size) {
  return buf_done(instance, ptr, size);
}

int main(int argc, char** argv) {
  /* Initialize the Wasm runtime. */
  wasm_rt_init();

  /* Initialize the rot13 module. */
  Z_rot13_init_module();

  /* Declare two instances of the `rot13` module. */
  Z_rot13_instance_t rot13_instance_1;
  Z_rot13_instance_t rot13_instance_2;

  /* Create two `host` module instances to store the memory and current string */
  struct Z_host_instance_t host_instance_1;
  struct Z_host_instance_t host_instance_2;
  /* Allocate 1 page of wasm memory (64KiB). */
  wasm_rt_allocate_memory(&host_instance_1.memory, 1, 1);
  wasm_rt_allocate_memory(&host_instance_2.memory, 1, 1);

  /* Construct the module instances */
  Z_rot13_instantiate(&rot13_instance_1, &host_instance_1);
  Z_rot13_instantiate(&rot13_instance_2, &host_instance_2);

  /* Call `rot13` on first two argument, using the mangled name. */
  assert(argc > 2);
  host_instance_1.input = argv[1];
  Z_rot13Z_rot13(&rot13_instance_1);
  host_instance_2.input = argv[2];
  Z_rot13Z_rot13(&rot13_instance_2);

  /* Free the rot13 modules. */
  Z_rot13_free(&rot13_instance_1);
  Z_rot13_free(&rot13_instance_2);

  /* Free the Wasm runtime state. */
  wasm_rt_free();

  return 0;
}

/* Fill the wasm buffer with the input to be rot13'd.
 *
 * params:
 *   ptr: The wasm memory address of the buffer to fill data.
 *   size: The size of the buffer in wasm memory.
 * result:
 *   The number of bytes filled into the buffer. (Must be <= size).
 */
u32 fill_buf(struct Z_host_instance_t* instance, u32 ptr, u32 size) {
  for (size_t i = 0; i < size; ++i) {
    if (instance->input[i] == 0) {
      return i;
    }
    instance->memory.data[ptr + i] = instance->input[i];
  }
  return size;
}

/* Called when the wasm buffer has been rot13'd.
 *
 * params:
 *   ptr: The wasm memory address of the buffer.
 *   size: The size of the buffer in wasm memory.
 */
void buf_done(struct Z_host_instance_t* instance, u32 ptr, u32 size) {
  /* The output buffer is not necessarily null-terminated, so use the %*.s
   * printf format to limit the number of characters printed. */
  printf("%s -> %.*s\n", instance->input, (int)size, &instance->memory.data[ptr]);
}
```
