    .section .text.init
    .globl _start
    .type _start, @function
_start:
    li t0, 4

    # The server's binutils predates RVV 1.0 assembly syntax, so these four
    # architecturally named instructions are emitted as their 32-bit encodings.
    .word 0x0102f357       # vsetvli t1,t0,e32,m1,tu,mu
    .word 0x5e01b0d7       # vmv.v.i v1,3
    .word 0x5e023157       # vmv.v.i v2,4
    .word 0x021101d7       # vadd.vv v3,v1,v2
    .word 0x423023d7       # vmv.x.s t2,v3

    li t3, 7
    bne t2, t3, .Lfail
    la t5, tohost
    li t6, 1
    sd t6, 0(t5)
.Lwait:
    j .Lwait

.Lfail:
    la t5, tohost
    li t6, 3
    sd t6, 0(t5)
    j .Lwait

    .size _start, .-_start

    .section .tohost,"aw",@progbits
    .balign 8
    .globl tohost
tohost:
    .dword 0
