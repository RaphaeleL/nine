.global _main
.align 2

_foo:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    adrp x0, Lstr0@PAGE
    add x0, x0, Lstr0@PAGEOFF
    bl _puts
    mov x0, #0
    ldp x29, x30, [sp], #16
    ret

_main:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    adrp x0, Lstr1@PAGE
    add x0, x0, Lstr1@PAGEOFF
    bl _puts
    bl _foo
    mov x0, #0
    ldp x29, x30, [sp], #16
    ret

Lstr0:
    .asciz "hello"

Lstr1:
    .asciz "Hello, World!"
