# Borrow Checker - Reference Types for Zen-C

Zen-C implements Rust-inspired reference types with compile-time borrow checking. References provide safe, zero-cost abstractions over C pointers — all safety checks happen at compile time, and the generated C code uses plain pointers with no runtime overhead.

## Reference Types

| Zen-C Syntax | Meaning | Generated C |
|--------------|---------|-------------|
| `&T` | Immutable (shared) reference | `const T*` |
| `&mut T` | Mutable (exclusive) reference | `T*` |
| `&[T]` | Borrowed slice (immutable) | `const Slice_T` |
| `&mut [T]` | Borrowed slice (mutable) | `Slice_T` |

## Rules

The borrow checker enforces three rules at compile time:

1. **Multiple shared borrows allowed**: You can have any number of `&T` references to the same variable simultaneously.

2. **Exclusive mutable access**: You can have exactly one `&mut T` reference to a variable at a time.

3. **No mixing**: You cannot have both `&T` and `&mut T` references to the same variable at the same time.

Additionally:
- Cannot assign through an immutable reference (`&T`)
- Cannot assign to a variable while it is borrowed
- Borrows are released when their scope exits

## Syntax

### Variable Declarations

```rust
let x: i32 = 42;

// Immutable borrow
let r: &i32 = &x;
let val: i32 = *r;    // read through reference

// Mutable borrow
let w: &mut i32 = &mut x;
*w = 100;              // write through reference

// Borrowed slice
let arr = [1, 2, 3, 4, 5];
let s: &[i32] = &arr;
```

### Function Parameters

```rust
fn read_value(x: &i32) -> i32 {
    return *x;
}

fn write_value(x: &mut i32, val: i32) {
    *x = val;
}

fn sum_array(data: &[i32]) -> i32 {
    let total: i32 = 0;
    for i in 0..data.len {
        total += data.data[i];
    }
    return total;
}
```

### Calling with References

```rust
let x: i32 = 10;
let val = read_value(&x);       // pass immutable reference
write_value(&mut x, 20);        // pass mutable reference

let nums = [1, 2, 3];
let total = sum_array(&nums);   // pass borrowed slice
```

### Struct References

```rust
struct Point {
    x: i32;
    y: i32;
}

fn print_point(p: &Point) {
    printf("(%d, %d)\n", p.x, p.y);
}

fn translate(p: &mut Point, dx: i32, dy: i32) {
    p.x = p.x + dx;
    p.y = p.y + dy;
}

let p = Point { x: 3, y: 4 };
print_point(&p);           // immutable borrow
translate(&mut p, 10, 20); // mutable borrow
```

## Compile-Time Errors

### Error: Double mutable borrow

```rust
let x: i32 = 10;
let a: &mut i32 = &mut x;
let b: &mut i32 = &mut x;  // ERROR: Cannot borrow 'x' as mutable more than once
```

### Error: Mixed borrows

```rust
let x: i32 = 10;
let r: &i32 = &x;
let w: &mut i32 = &mut x;  // ERROR: Cannot borrow 'x' as mutable while borrowed as immutable
```

### Error: Write through immutable reference

```rust
let x: i32 = 10;
let r: &i32 = &x;
*r = 20;                    // ERROR: Cannot assign through immutable reference 'r'
```

### Error: Assign to borrowed variable

```rust
let x: i32 = 10;
let r: &i32 = &x;
x = 20;                     // ERROR: Cannot assign to 'x' while it is borrowed
```

## Scope-Based Release

Borrows are automatically released when their scope ends:

```rust
let x: i32 = 10;

{
    let r: &i32 = &x;       // borrow starts
    // use r...
}                            // borrow released here

let w: &mut i32 = &mut x;   // OK - no active borrows
*w = 20;
```

## Generated C Code

Zen-C references compile to clean C with no runtime overhead:

### Input (Zen-C)

```rust
fn increment(x: &mut i32) {
    *x = *x + 1;
}

fn read(x: &i32) -> i32 {
    return *x;
}

fn main() -> i32 {
    let counter: i32 = 0;
    increment(&mut counter);
    let val: i32 = read(&counter);
    return val;
}
```

### Output (C)

```c
void increment(int32_t* x) {
    *x = *x + 1;
}

int32_t read(const int32_t* x) {
    return *x;
}

int main(void) {
    int32_t counter = 0;
    increment(&counter);
    int32_t val = read(&counter);
    return val;
}
```

## Limitations vs Rust

| Feature | Zen-C | Rust |
|---------|-------|------|
| Scope-based borrows | Yes | Yes |
| Lifetime annotations (`'a`) | No | Yes |
| Cross-function lifetime tracking | No | Yes |
| Non-lexical lifetimes (NLL) | No | Yes |
| Self-referential structs | No | Yes (with Pin) |
| Struct field splitting (`&mut s.x` + `&s.y`) | No | Yes |
| Thread safety (`Send`/`Sync`) | No | Yes |

Zen-C's borrow checker catches ~60-70% of common memory safety bugs (double mutable aliasing, mutation through shared references, use of borrowed-then-modified data). For full memory safety guarantees, use Rust.

## Integration with Existing Features

- **Move semantics**: Works alongside the existing `is_moved` tracking in the type checker
- **Auto-deref**: The `.` operator auto-dereferences through references (same as pointers)
- **C interop**: `&T` is compatible with `T*` from C headers — you can pass references to C functions expecting pointers
- **Raw blocks**: Inside `raw {}` blocks, borrow rules are not enforced (escape hatch)
